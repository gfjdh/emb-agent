import cv2
#选择此代码目录下的某个图片文件
img = cv2.imread('lena.png',1)
#img = cv2.imread('zuqiu.jpeg',1)
grayimg = cv2.cvtColor(img,cv2.COLOR_BGR2GRAY)#图像灰度化，用于简化矩阵运算
# 导入人脸级联分类器，'.xml'文件里包含训练出来的人脸特征
face_engine = cv2.CascadeClassifier('/usr/share/opencv4/haarcascades/haarcascade_frontalface_alt.xml')
# 检测设置，将图片放大1.1倍（一般设1.1倍，看效果而定）
# 重复检测的次数为5次（检测次数越多，速度越慢，检测也越严格，准确率可能有所提升）
faces = face_engine.detectMultiScale(grayimg,scaleFactor=1.1,minNeighbors=5)
# 对图片进行人脸检测，之后得到人脸的坐标（一个矩形框），再用蓝色框框出，线宽为2
for (x,y,w,h) in faces:
    img = cv2.rectangle(img,(x,y),(x+w,y+h),(255,0,0),2)  # （255,0，0）=（B，G，R）
# 显示图片
cv2.imshow('img',img)
# 检测是否有按键按下
cv2.waitKey(0)
# 关闭窗口，释放占用的资源
cv2.destroyAllWindows()
