"""
实验五: 外设控制实验
测试手势识别到GPIO外设控制的完整链路
"""

import sys
import time
from pathlib import Path
from collections import defaultdict
import cv2

sys.path.insert(0, str(Path(__file__).parent.parent))

from src.capture import Camera
from src.detection import HandDetector
from src.features import FeatureExtractor
from src.classifier import KNNClassifier
from src.control import GPIOController


def main():
    print("=" * 60)
    print("实验五: 外设控制实验")
    print("=" * 60)

    model_path = Path("data/models/knn_model.pkl")
    if not model_path.exists():
        print("[ERROR] 请先训练模型: python scripts/train.py")
        return

    classifier = KNNClassifier(k=5)
    classifier.load(str(model_path))

    detector = HandDetector(static_image_mode=False, max_num_hands=1)
    extractor = FeatureExtractor()
    controller = GPIOController(confirm_frames=5)
    camera = Camera(camera_id=0)

    # 统计数据
    stats = defaultdict(lambda: {"recognized": 0, "controlled": 0, "total_latency": 0.0})

    try:
        if not camera.open():
            print("[ERROR] 无法打开摄像头")
            return

        print("[INFO] 按 'q' 退出")
        print("[INFO] 切换手势后需保持5帧以上触发控制")

        while True:
            success, frame = camera.read()
            if not success:
                continue

            frame = cv2.flip(frame, 1)

            detected, landmarks = detector.detect(frame)

            if detected and landmarks is not None:
                detector.draw_landmarks(frame, landmarks)

                features = extractor.extract(landmarks, "all")
                t1 = time.time()
                label, confidence = classifier.predict_with_confidence(features)
                t2 = time.time()

                gesture_name = classifier.get_label_name(label)
                stats[gesture_name]["recognized"] += 1

                action_desc = controller.execute(gesture_name)
                if action_desc:
                    stats[gesture_name]["controlled"] += 1
                    stats[gesture_name]["total_latency"] += (t2 - t1) * 1000
                    cv2.putText(frame, f">> {action_desc}", (10, 115),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 165, 255), 2)

                cv2.putText(frame, f"{gesture_name} ({confidence:.2f})", (10, 55),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)

            cv2.putText(frame, "Exp 5: Control Test | q=exit", (10, 25),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 255), 2)
            cv2.imshow("Experiment 5: Control Test", frame)

            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

    except KeyboardInterrupt:
        pass
    finally:
        camera.release()
        detector.close()
        controller.cleanup()
        cv2.destroyAllWindows()

        print("\n" + "=" * 60)
        print("实验结果")
        print("=" * 60)
        print(f"{'手势':<10} {'识别次数':<10} {'控制次数':<10} {'成功率':<10} {'平均延迟':<12}")
        print("-" * 55)

        total_recognized = 0
        total_controlled = 0
        for gesture, s in stats.items():
            rate = s["controlled"] / s["recognized"] * 100 if s["recognized"] > 0 else 0
            avg_latency = s["total_latency"] / s["controlled"] if s["controlled"] > 0 else 0
            print(f"{gesture:<10} {s['recognized']:<10} {s['controlled']:<10} "
                  f"{rate:<9.1f}% {avg_latency:<10.1f}ms")
            total_recognized += s["recognized"]
            total_controlled += s["controlled"]

        overall_rate = total_controlled / total_recognized * 100 if total_recognized > 0 else 0
        print("-" * 55)
        print(f"{'合计':<10} {total_recognized:<10} {total_controlled:<10} {overall_rate:<9.1f}%")


if __name__ == "__main__":
    main()
