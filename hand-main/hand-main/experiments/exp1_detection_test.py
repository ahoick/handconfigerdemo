"""
实验一: MediaPipe关键点检测实验
验证 MediaPipe 在嵌入式平台上的检测能力
"""

import sys
import time
from pathlib import Path
import cv2

sys.path.insert(0, str(Path(__file__).parent.parent))

from src.capture import Camera
from src.detection import HandDetector


def main():
    print("=" * 60)
    print("实验一: MediaPipe 关键点检测实验")
    print("=" * 60)

    detector = HandDetector(
        static_image_mode=False,
        max_num_hands=1,
        min_detection_confidence=0.7,
    )
    camera = Camera(camera_id=0)

    try:
        if not camera.open():
            print("[ERROR] 无法打开摄像头")
            return

        print("[INFO] 按 'q' 退出")

        total_frames = 0
        detected_frames = 0
        start_time = time.time()

        while True:
            success, frame = camera.read()
            if not success:
                continue

            frame = cv2.flip(frame, 1)
            total_frames += 1

            # 检测
            detected, landmarks = detector.detect(frame)

            if detected and landmarks is not None:
                detected_frames += 1
                detector.draw_landmarks(frame, landmarks)

            # FPS
            elapsed = time.time() - start_time
            fps = total_frames / elapsed if elapsed > 0 else 0
            detect_rate = detected_frames / total_frames * 100 if total_frames > 0 else 0

            cv2.putText(frame, f"FPS: {fps:.1f}", (10, 25),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)
            cv2.putText(frame, f"Detection Rate: {detect_rate:.1f}%", (10, 55),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)
            cv2.putText(frame, f"Frames: {total_frames} Detected: {detected_frames}",
                        (10, 80), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (200, 200, 200), 1)

            cv2.imshow("Experiment 1: Detection Test", frame)

            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

    except KeyboardInterrupt:
        pass
    finally:
        elapsed = time.time() - start_time
        camera.release()
        detector.close()
        cv2.destroyAllWindows()

        print("\n" + "=" * 60)
        print("实验结果")
        print("=" * 60)
        print(f"总帧数:       {total_frames}")
        print(f"检测成功:     {detected_frames}")
        print(f"检测成功率:   {detected_frames/total_frames*100:.1f}%")
        print(f"总时间:       {elapsed:.1f}s")
        print(f"平均帧率:     {total_frames/elapsed:.1f} FPS")
        print(f"检测帧率:     {detected_frames/elapsed:.1f} FPS")


if __name__ == "__main__":
    main()
