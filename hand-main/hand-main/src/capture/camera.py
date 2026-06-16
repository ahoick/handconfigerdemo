"""
图像采集模块 - Camera Capture Module
通过 USB 摄像头实时采集视频帧
"""

import cv2
from typing import Optional, Tuple


class Camera:
    """USB摄像头采集器"""

    def __init__(self, camera_id: int = 0, width: int = 640, height: int = 480):
        """
        初始化摄像头

        Args:
            camera_id: 摄像头设备ID，默认0
            width:    采集宽度
            height:   采集高度
        """
        self.camera_id = camera_id
        self.width = width
        self.height = height
        self.cap: Optional[cv2.VideoCapture] = None

    def open(self) -> bool:
        """打开摄像头"""
        self.cap = cv2.VideoCapture(self.camera_id)
        if not self.cap.isOpened():
            return False
        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, self.width)
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, self.height)
        return True

    def read(self) -> Tuple[bool, Optional[any]]:
        """
        读取一帧

        Returns:
            (success, frame): 成功标志与图像帧
        """
        if self.cap is None:
            return False, None
        return self.cap.read()

    def release(self):
        """释放摄像头"""
        if self.cap is not None:
            self.cap.release()
            self.cap = None

    def __enter__(self):
        self.open()
        return self

    def __exit__(self, *args):
        self.release()
