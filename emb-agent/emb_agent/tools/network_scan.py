"""Network scanning tool to discover target board IP addresses."""

import asyncio
import re
from typing import Any

from emb_agent.tools.base import Tool


class NetworkScanTool(Tool):
    """Tool to scan local network and discover target board IP addresses."""

    @property
    def name(self) -> str:
        return "scan_network"

    @property
    def description(self) -> str:
        return "Scan local network to discover available devices and find target board IP addresses. Supports discovering Phytium (飞腾派) boards by hostname patterns."

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
        subnet: str = "192.168.1.0/24",
        timeout: int = 30,
        target_pattern: str = "phyt|ubuntu|pi",
        **kwargs: Any,
    ) -> str:
        """Scan network and discover devices."""
        results = []

        # 方法1：使用arp扫描
        arp_result = await self._scan_arp()
        if arp_result:
            results.append("=== ARP扫描结果 ===")
            results.append(arp_result)

        # 方法2：使用ping扫描
        ping_result = await self._scan_ping(subnet, timeout)
        if ping_result:
            results.append("\n=== Ping扫描结果 ===")
            results.append(ping_result)

        # 方法3：查找mDNS主机名
        mdns_result = await self._scan_mdns()
        if mdns_result:
            results.append("\n=== mDNS主机发现 ===")
            results.append(mdns_result)

        # 筛选匹配目标模式的设备
        target_devices = self._filter_targets(results, target_pattern)
        if target_devices:
            results.append("\n=== 发现的目标设备 ===")
            results.append("\n".join(f"- {device}" for device in target_devices))
            results.append(f"\n建议使用IP: {target_devices[0].split()[0]}")

        return "\n".join(results) if results else "未发现网络设备"

    async def _scan_arp(self) -> str:
        """Execute ARP scan to find devices on the network."""
        try:
            proc = await asyncio.create_subprocess_shell(
                "arp -a",
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
            )
            stdout, _ = await proc.communicate()
            output = stdout.decode("utf-8", errors="replace")
            lines = output.strip().split("\n")
            if lines and len(lines) > 1:
                return "\n".join(lines)
            return ""
        except Exception:
            return ""

    async def _scan_ping(self, subnet: str, timeout: int) -> str:
        """Execute ping scan to find active hosts."""
        try:
            import ipaddress
            
            network = ipaddress.ip_network(subnet, strict=False)
            hosts = list(network.hosts())[:50]  # 限制扫描数量
            
            async def ping_host(ip):
                try:
                    proc = await asyncio.create_subprocess_shell(
                        f"ping -c 1 -W 1 {ip}",
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
            # 尝试使用avahi-browse或dns-sd
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
        for result in results:
            lines = result.split("\n")
            for line in lines:
                if re.search(pattern, line, re.IGNORECASE):
                    # 提取IP地址和主机名
                    ip_match = re.search(r'(\d+\.\d+\.\d+\.\d+)', line)
                    if ip_match:
                        ip = ip_match.group(1)
                        hostname = re.search(r'([a-zA-Z0-9-]+)\.local', line)
                        host_str = hostname.group(1) if hostname else "unknown"
                        targets.append(f"{ip} ({host_str})")
        return list(dict.fromkeys(targets))  # 去重


class GetBoardIPTool(Tool):
    """Tool to get the IP address of the target board automatically."""

    @property
    def name(self) -> str:
        return "get_board_ip"

    @property
    def description(self) -> str:
        return "Automatically discover and return the IP address of the target Phytium board on the local network."

    @property
    def parameters(self) -> dict[str, Any]:
        return {
            "type": "object",
            "properties": {
                "subnet": {
                    "type": "string",
                    "description": "Network subnet to scan (default: 192.168.1.0/24)",
                },
            },
            "required": [],
        }

    async def execute(self, subnet: str = "192.168.1.0/24", **kwargs: Any) -> str:
        """Get the IP address of the target board."""
        scanner = NetworkScanTool()
        result = await scanner.execute(subnet=subnet, timeout=20)
        
        # 提取发现的目标设备IP
        lines = result.split("\n")
        for line in lines:
            if "发现的目标设备" in line:
                continue
            ip_match = re.search(r'(\d+\.\d+\.\d+\.\d+)', line)
            if ip_match:
                return ip_match.group(1)
        
        return "未找到目标设备，请检查网络连接或尝试手动指定IP"
