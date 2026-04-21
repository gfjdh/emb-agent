import cv2
# 导入人脸级联分类器，'.xml'文件里包含训练出来的人脸特征
face_engine = cv2.CascadeClassifier('/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml')
# 导入人眼级联分类器，'.xml'文件里包含训练出来的人眼特征
eye_cascade = cv2.CascadeClassifier('/usr/share/opencv4/haarcascades/haarcascade_eye.xml')
# 调用摄像头摄像头
cap = cv2.VideoCapture(0)
while(True):
    # 获取摄像头拍摄到的画面
    # 会得到两个参数，一个为存放是否捕捉到图像（True/False），另一个为存放每帧的图像
    ret, frame = cap.read()
    # 每帧图像放大1.1倍，重复检测10次
    faces = face_engine.detectMultiScale(frame,1.1, 10)
    img = frame
    for (x,y,w,h) in faces:
        # 画出人脸框，蓝色，画笔宽度为2
        img = cv2.rectangle(img,(x,y),(x+w,y+h),(255,0,0),2)
        # 框选出人脸区域，在人脸区域而不是全图中进行人眼检测，节省计算资源
        face_area = img[y:y+h, x:x+w]
        eyes = eye_cascade.detectMultiScale(face_area,1.1,5)
        # 用人眼级联分类器在人脸区域进行人眼识别，返回的eyes为眼睛坐标列表
        for (ex,ey,ew,eh) in eyes:
            #画出人眼框，绿色，画笔宽度为1
            cv2.rectangle(face_area,(ex,ey),(ex+ew,ey+eh),(0,255,0),1)
    # 实时展示效果画面
    cv2.imshow('frame2',img)
    # 每5毫秒监听一次键盘动作
    if cv2.waitKey(5) & 0xFF == ord('a'):  #当按下“a”键时退出人脸检测
        break
# 最后，关闭所有窗口
cap.release()
cv2.destroyAllWindows()
