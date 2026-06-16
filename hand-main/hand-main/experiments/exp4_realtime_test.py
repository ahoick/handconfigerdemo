"""
实验四: 实时识别实验
在嵌入式平台运行完整系统，统计实时性能指标
"""

import sys
import time
from pathlib import Path
import cv2

sys.path.insert(0, str(Path(__file__).parent.parent))

from src.capture import Camera
from src.detection import HandDetector
from src.features import FeatureExtractor
from src.classifier import KNNClassifier


def main():
    print("=" * 60)
    print("实验四: 实时识别实验")
    print("=" * 60)

    model_path = Path("data/models/knn_model.pkl")
    if not model_path.exists():
        print("[ERROR] 请先训练模型: python scripts/train.py")
        return

    classifier = KNNClassifier(k=5)
    classifier.load(str(model_path))

    detector = HandDetector(static_image_mode=False, max_num_hands=1)
    extractor = FeatureExtractor()
    camera = Camera(camera_id=0)

    try:
        if not camera.open():
            print("[ERROR] 无法打开摄像头")
            return

        print("[INFO] 按 'q' 退出")

        total_frames = 0
        detected_frames = 0
        total_detect_time = 0.0
        total_classify_time = 0.0
        start_time = time.time()

        while True:
            success, frame = camera.read()
            if not success:
                continue

            frame = cv2.flip(frame, 1)
            total_frames += 1

            # 检测 + 计时
            t1 = time.time()
            detected, landmarks = detector.detect(frame)
            t2 = time.time()
            detect_time = (t2 - t1) * 1000  # ms

            if detected and landmarks is not None:
                detected_frames += 1
                detector.draw_landmarks(frame, landmarks)

                # 分类 + 计时
                features = extractor.extract(landmarks, "all")
                t3 = time.time()
                label, confidence = classifier.predict_with_confidence(features)
                t4 = time.time()
                classify_time = (t4 - t3) * 1000

                total_detect_time += detect_time
                total_classify_time += classify_time

                gesture_name = classifier.get_label_name(label)
                cv2.putText(frame, f"{gesture_name} ({confidence:.2f})", (10, 55),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
                cv2.putText(frame, f"Det: {detect_time:.1f}ms  Cls: {classify_time:.1f}ms",
                            (10, 85), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 200, 0), 1)

            # FPS
            elapsed = time.time() - start_time
            fps = total_frames / elapsed if elapsed > 0 else 0

            cv2.putText(frame, f"FPS: {fps:.1f}", (10, 25),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)

            cv2.imshow("Experiment 4: Real-time Test", frame)

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
        print(f"总帧数:           {total_frames}")
        print(f"检测成功:         {detected_frames}")
        print(f"检测成功率:       {detected_frames/total_frames*100:.1f}%")
        print(f"总运行时间:       {elapsed:.1f}s")
        print(f"实时帧率:         {total_frames/elapsed:.1f} FPS")
        if detected_frames > 0:
            print(f"平均检测耗时:     {total_detect_time/detected_frames:.1f} ms")
            print(f"平均分类耗时:     {total_classify_time/detected_frames:.1f} ms")
            print(f"平均总耗时:       {(total_detect_time+total_classify_time)/detected_frames:.1f} ms")


if __name__ == "__main__":
    main()
