"""Network scanning tool to discover target board IP addresses."""

import asyncio
import os
import re
import socket
from typing import Any, Tuple

from emb_agent.tools.base import Tool

# 检测操作系统
IS_WINDOWS = os.name == "nt"

# 默认目标设备配置
TARGET_HOSTNAME = "Phytium-Pi"
TARGET_IP = "10.222.248.225"
TARGET_MAC = "f4-3c-3b-f8-6e-10"

def get_local_subnets() -> list[str]:
    """Get local network subnets from active network interfaces, prioritizing private networks."""
    subnets = []
    try:
        if IS_WINDOWS:
            import subprocess
            result = subprocess.run(
                        ["ipconfig"], capture_output=True, text=True, timeout=10
                    )
            lines = result.stdout.split("\n")
            
            for line in lines:
                if "IPv4 Address" in line:
                    match = re.search(r'(\d+\.\d+\.\d+\.\d+)', line)
                    if match:
                        ip = match.group(1)
                        if ip.startswith("127.") or ip.startswith("169.254."):
                            continue
                        
                        if (ip.startswith("10.") or 
                            (ip.startswith("172.") and 16 <= int(ip.split(".")[1]) <= 31) or
                            ip.startswith("192.168.")):
                            parts = ip.split(".")
                            subnet = f"{parts[0]}.{parts[1]}.{parts[2]}.0/24"
                            subnets.append(subnet)
        else:
            interfaces = socket.if_nameindex()
            for iface in interfaces:
                try:
                    addresses = socket.getaddrinfo(
                        socket.gethostname(), None, socket.AF_INET
                    )
                    for addr in addresses:
                        ip = addr[4][0]
                        if ip.startswith("127."):
                            continue
                        if (ip.startswith("10.") or 
                            (ip.startswith("172.") and 16 <= int(ip.split(".")[1]) <= 31) or
                            ip.startswith("192.168.")):
                            parts = ip.split(".")
                            subnet = f"{parts[0]}.{parts[1]}.{parts[2]}.0/24"
                            subnets.append(subnet)
                except Exception:
                    continue
        
        result = list(set(subnets))
        return result if result else ["192.168.1.0/24", "192.168.0.0/24"]
    except Exception:
        return ["192.168.1.0/24", "192.168.0.0/24"]

def get_local_ips() -> list[str]:
    """Get all local IP addresses of the current machine."""
    ips = []
    try:
        if IS_WINDOWS:
            import subprocess
            result = subprocess.run(
                        ["ipconfig"], capture_output=True, text=True, timeout=10
                    )
            lines = result.stdout.split("\n")
            for line in lines:
                if "IPv4 Address" in line:
                    match = re.search(r'(\d+\.\d+\.\d+\.\d+)', line)
                    if match:
                        ip = match.group(1)
                        if not ip.startswith("127.") and not ip.startswith("169.254."):
                            ips.append(ip)
        else:
            hostname = socket.gethostname()
            addresses = socket.getaddrinfo(hostname, None, socket.AF_INET)
            for addr in addresses:
                ip = addr[4][0]
                if not ip.startswith("127."):
                    ips.append(ip)
        return list(set(ips))
    except Exception:
        return []

async def get_arp_table() -> list[Tuple[str, str]]:
    """Get ARP table entries (IP -> MAC mapping)."""
    entries = []
    try:
        if IS_WINDOWS:
            proc = await asyncio.create_subprocess_shell(
                "arp -a",
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
            )
            stdout, _ = await proc.communicate()
            output = stdout.decode("utf-8", errors="replace")
            lines = output.strip().split("\n")
            
            for line in lines:
                ip_match = re.search(r'(\d+\.\d+\.\d+\.\d+)', line)
                mac_match = re.search(r'([0-9A-Fa-f]{2}[-:][0-9A-Fa-f]{2}[-:][0-9A-Fa-f]{2}[-:][0-9A-Fa-f]{2}[-:][0-9A-Fa-f]{2}[-:][0-9A-Fa-f]{2})', line)
                if ip_match and mac_match:
                    ip = ip_match.group(1)
                    mac = mac_match.group(1).lower()
                    if ip and mac != "ff-ff-ff-ff-ff-ff":
                        entries.append((ip, mac))
        else:
            proc = await asyncio.create_subprocess_shell(
                "arp -n",
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
            )
            stdout, _ = await proc.communicate()
            output = stdout.decode("utf-8", errors="replace")
            lines = output.strip().split("\n")
            
            for line in lines:
                parts = line.split()
                if len(parts) >= 4:
                    ip = parts[0]
                    mac = parts[3].lower()
                    if ip and mac != "ff:ff:ff:ff:ff:ff" and mac != "00:00:00:00:00:00":
                        entries.append((ip, mac))
        return entries
    except Exception as e:
        print(f"Error getting ARP table: {e}")
        return entries

async def get_mac_address_by_hostname(hostname: str) -> str:
    """Get MAC address by resolving hostname and checking ARP table."""
    try:
        ip_address = socket.gethostbyname(hostname)
        arp_entries = await get_arp_table()
        for ip, mac in arp_entries:
            if ip == ip_address:
                return mac
        return ""
    except Exception:
        return ""

async def scan_subnet_for_mac(subnet: str, target_mac: str) -> str:
    """Scan a subnet to find IP associated with a specific MAC address."""
    try:
        import ipaddress
        network = ipaddress.ip_network(subnet, strict=False)
        hosts = list(network.hosts())[:254]
        
        async def ping_host(ip):
            try:
                if IS_WINDOWS:
                    cmd = f"ping -n 1 -w 500 {ip}"
                else:
                    cmd = f"ping -c 1 -W 1 {ip}"
                
                proc = await asyncio.create_subprocess_shell(
                    cmd,
                    stdout=asyncio.subprocess.PIPE,
                    stderr=asyncio.subprocess.PIPE,
                )
                await proc.communicate()
                return str(ip) if proc.returncode == 0 else None
            except Exception:
                return None
        
        tasks = [ping_host(str(host)) for host in hosts]
        await asyncio.gather(*tasks)
        
        arp_entries = await get_arp_table()
        for ip, mac in arp_entries:
            if mac == target_mac.lower() or target_mac.lower() in mac:
                return ip
        return ""
    except Exception as e:
        print(f"Error scanning subnet: {e}")
        return ""


class NetworkScanTool(Tool):
    """Tool to scan local network and discover target board IP addresses."""

    @property
    def name(self) -> str:
        return "scan_network"

    @property
    def description(self) -> str:
        return "Scan local network to discover available devices and find target board IP addresses. Supports discovering Phytium (飞腾派) boards by hostname patterns including phyt, ubuntu, pi, phytium."

    @property
    def parameters(self) -> dict[str, Any]:
        return {
            "type": "object",
            "properties": {
                "subnet": {
                    "type": "string",
                    "description": "Network subnet to scan (default: 192.168.1.0/24)",
                },
                "timeout": {
                    "type": "integer",
                    "description": "Scan timeout in seconds (default: 30)",
                    "minimum": 5,
                    "maximum": 120,
                },
                "target_pattern": {
                    "type": "string",
                    "description": "Hostname pattern to match (default: phyt,ubuntu,pi)",
                },
            },
            "required": [],
        }

    async def execute(
        self,
        subnet: str = "",
        timeout: int = 30,
        target_pattern: str = "phyt|ubuntu|pi|phytium",
        **kwargs: Any,
    ) -> str:
        """Scan network and discover devices using multiple methods."""
        results = []

        arp_result = await self._scan_arp()
        if arp_result:
            results.append("=== ARP扫描结果 ===")
            results.append(arp_result)
        else:
            results.append("=== ARP扫描结果 ===")
            results.append("未发现ARP设备")

        mdns_result = await self._scan_mdns()
        if mdns_result:
            results.append("\n=== mDNS主机发现 ===")
            results.append(mdns_result)

        target_devices = self._filter_targets(results, target_pattern)
        if target_devices:
            results.append("\n=== 发现的目标设备 ===")
            results.append("\n".join(f"- {device}" for device in target_devices))
            results.append(f"\n建议使用IP: {target_devices[0].split()[0]}")

        return "\n".join(results) if results else "未发现网络设备"

    async def _scan_arp(self) -> str:
        """Execute ARP scan to find devices on the network."""
        try:
            arp_entries = await get_arp_table()
            if arp_entries:
                return "\n".join(f"{ip} -> {mac}" for ip, mac in arp_entries)
            return ""
        except Exception:
            return ""

    async def _scan_ping(self, subnet: str, timeout: int) -> str:
        """Execute ping scan to find active hosts."""
        try:
            import ipaddress
            
            network = ipaddress.ip_network(subnet, strict=False)
            hosts = list(network.hosts())[:254]
            
            async def ping_host(ip):
                try:
                    if IS_WINDOWS:
                        cmd = f"ping -n 1 -w 1000 {ip}"
                    else:
                        cmd = f"ping -c 1 -W 1 {ip}"
                    
                    proc = await asyncio.create_subprocess_shell(
                        cmd,
                        stdout=asyncio.subprocess.PIPE,
                        stderr=asyncio.subprocess.PIPE,
                    )
                    await proc.communicate()
                    return str(ip) if proc.returncode == 0 else None
                except Exception:
                    return None

            tasks = [ping_host(str(host)) for host in hosts]
            results = await asyncio.gather(*tasks)
            active_hosts = [r for r in results if r]
            
            if active_hosts:
                return "活跃主机: " + ", ".join(active_hosts)
            return ""
        except Exception:
            return ""

    async def _scan_mdns(self) -> str:
        """Scan for mDNS/Bonjour hosts."""
        try:
            for cmd in ["avahi-browse -al", "dns-sd -B _http._tcp local"]:
                try:
                    proc = await asyncio.create_subprocess_shell(
                        cmd,
                        stdout=asyncio.subprocess.PIPE,
                        stderr=asyncio.subprocess.PIPE,
                    )
                    stdout, _ = await asyncio.wait_for(proc.communicate(), timeout=5)
                    output = stdout.decode("utf-8", errors="replace")
                    if output.strip():
                        return output
                except (asyncio.TimeoutError, FileNotFoundError):
                    continue
            return ""
        except Exception:
            return ""

    def _filter_targets(self, results: list[str], pattern: str) -> list[str]:
        """Filter results to find devices matching target pattern."""
        targets = []
        local_ips = get_local_ips()
        
        for result in results:
            lines = result.split("\n")
            for line in lines:
                ip_match = re.search(r'(\d+\.\d+\.\d+\.\d+)', line)
                if ip_match:
                    ip = ip_match.group(1)
                    if ip in local_ips:
                        continue
                    
                    hostname = re.search(r'([a-zA-Z0-9-]+)\.local', line)
                    host_str = hostname.group(1) if hostname else "unknown"
                    
                    if host_str != "unknown":
                        if re.search(pattern, host_str, re.IGNORECASE):
                            targets.append(f"{ip} ({host_str})")
                    elif re.search(pattern, line, re.IGNORECASE):
                        targets.append(f"{ip} ({host_str})")
        return list(dict.fromkeys(targets))


class GetBoardIPTool(Tool):
    """Tool to get the IP address of the target board automatically using MAC address matching."""

    @property
    def name(self) -> str:
        return "get_board_ip"

    @property
    def description(self) -> str:
        return "Automatically discover and return the IP address of the target Phytium board on the local network using MAC address matching."

    @property
    def parameters(self) -> dict[str, Any]:
        return {
            "type": "object",
            "properties": {
                "subnet": {
                    "type": "string",
                    "description": "Network subnet to scan (default: auto-detect)",
                },
                "target_mac": {
                    "type": "string",
                    "description": "Target MAC address to match (optional, will auto-detect if not provided)",
                },
            },
            "required": [],
        }

    async def execute(self, subnet: str = "", target_mac: str = "", **kwargs: Any) -> str:
        """Get the IP address of the target board using MAC address matching."""
        results = []
        
        results.append("=== 开始查找目标设备 ===")
        results.append(f"目标主机名: {TARGET_HOSTNAME}")
        results.append(f"默认目标IP: {TARGET_IP}")
        results.append(f"目标MAC地址: {TARGET_MAC}")
        
        if target_mac:
            results.append(f"指定的覆盖MAC: {target_mac}")
        
        target_mac_address = target_mac if target_mac else TARGET_MAC
        
        if target_mac_address:
            results.append(f"\n1. 使用MAC地址 {target_mac_address} 查找IP...")
            arp_entries = await get_arp_table()
            
            for ip, mac in arp_entries:
                if target_mac_address.lower() == mac.lower() or target_mac_address.lower() in mac:
                    results.append(f"   在ARP表中找到匹配: {ip} -> {mac}")
                    return f"成功找到目标设备！\nIP地址: {ip}\nMAC地址: {mac}"
            
            results.append("   ARP表中未找到匹配，开始扫描子网...")
            
            subnets_to_scan = [subnet] if subnet else get_local_subnets()
            
            for sub in subnets_to_scan:
                results.append(f"   扫描子网: {sub}")
                found_ip = await scan_subnet_for_mac(sub, target_mac_address)
                if found_ip:
                    results.append(f"   在子网 {sub} 中找到: {found_ip}")
                    return f"成功找到目标设备！\nIP地址: {found_ip}\nMAC地址: {target_mac_address}"
            
            results.append("   所有子网扫描完成，未找到匹配设备")
        
        results.append("\n2. 尝试直接使用默认IP地址...")
        results.append(f"   默认IP: {TARGET_IP}")
        
        try:
            if IS_WINDOWS:
                cmd = f"ping -n 1 -w 1000 {TARGET_IP}"
            else:
                cmd = f"ping -c 1 -W 1 {TARGET_IP}"
            
            proc = await asyncio.create_subprocess_shell(
                cmd,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
            )
            await proc.communicate()
            
            if proc.returncode == 0:
                results.append(f"   默认IP {TARGET_IP} 可达")
                return f"找到目标设备！\nIP地址: {TARGET_IP}\n主机名: {TARGET_HOSTNAME}"
            else:
                results.append(f"   默认IP {TARGET_IP} 不可达")
        except Exception as e:
            results.append(f"   ping测试失败: {e}")
        
        results.append("\n3. 执行通用网络扫描...")
        scanner = NetworkScanTool()
        
        subnets_to_scan = [subnet] if subnet else get_local_subnets()
        all_targets = []
        
        for sub in subnets_to_scan:
            results.append(f"   扫描子网: {sub}")
            result = await scanner.execute(subnet=sub, timeout=20)
            
            lines = result.split("\n")
            in_target_section = False
            for line in lines:
                if "发现的目标设备" in line:
                    in_target_section = True
                    continue
                if in_target_section and line.strip().startswith("- "):
                    ip_match = re.search(r'(\d+\.\d+\.\d+\.\d+)', line)
                    if ip_match:
                        all_targets.append(line.strip()[2:])
        
        if all_targets:
            results.append(f"   发现 {len(all_targets)} 个候选设备")
            for target in all_targets:
                results.append(f"      - {target}")
            
            phytium_targets = [t for t in all_targets if re.search(r'Phytium|phytpi|Pi', t, re.IGNORECASE)]
            if phytium_targets:
                ip_match = re.search(r'(\d+\.\d+\.\d+\.\d+)', phytium_targets[0])
                ip = ip_match.group(1) if ip_match else phytium_targets[0].split()[0]
                results.append(f"\n推荐使用: {ip}")
                return f"找到目标设备！\nIP地址: {ip}"
            elif len(all_targets) == 1:
                ip_match = re.search(r'(\d+\.\d+\.\d+\.\d+)', all_targets[0])
                ip = ip_match.group(1) if ip_match else all_targets[0].split()[0]
                return f"找到目标设备！\nIP地址: {ip}"
            else:
                return "发现多个候选设备，请手动选择：\n" + "\n".join(f"- {t}" for t in all_targets)
        
        results.append("   未发现候选设备")
        return "未找到目标设备，请检查网络连接或尝试手动指定IP"
