"""
数据集采集脚本 - Data Collection Script
通过摄像头采集手势样本图片，建立自定义数据集
"""

import sys
import time
import argparse
from pathlib import Path
import cv2

sys.path.insert(0, str(Path(__file__).parent.parent))

from src.capture import Camera


def parse_args():
    parser = argparse.ArgumentParser(description="手势数据集采集")
    parser.add_argument("--gesture", type=str, required=True,
                        help="手势名称，如: 数字1, OK手势, 握拳手势")
    parser.add_argument("--count", type=int, default=400,
                        help="采集数量")
    parser.add_argument("--output", type=str, default="data/raw",
                        help="输出目录")
    parser.add_argument("--camera", type=int, default=0,
                        help="摄像头ID")
    parser.add_argument("--interval", type=float, default=0.1,
                        help="采集间隔(秒)")
    return parser.parse_args()


def main():
    args = parse_args()

    # 创建手势目录
    gesture_dir = Path(args.output) / args.gesture
    gesture_dir.mkdir(parents=True, exist_ok=True)

    # 计算已有文件数，从已有编号后继续
    existing = list(gesture_dir.glob("*.jpg"))
    start_idx = len(existing)
    print(f"[INFO] 手势: {args.gesture}")
    print(f"[INFO] 已有: {start_idx} 张, 目标: {args.count} 张")
    print(f"[INFO] 输出: {gesture_dir}")

    with Camera(camera_id=args.camera) as camera:
        print("[INFO] 按 SPACE 开始采集，按 'q' 退出")
        collecting = False
        idx = start_idx

        while idx < start_idx + args.count:
            success, frame = camera.read()
            if not success:
                continue

            frame = cv2.flip(frame, 1)

            # 显示信息
            cv2.putText(frame, f"Gesture: {args.gesture}", (10, 25),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
            cv2.putText(frame, f"Progress: {idx - start_idx}/{args.count}", (10, 55),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)

            if collecting:
                cv2.putText(frame, "RECORDING...", (10, 85),
                            cv2.FONT_HERSHEY_SIMPLEX, 1.0, (0, 0, 255), 2)
            else:
                cv2.putText(frame, "PAUSED (SPACE to start)", (10, 85),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 0), 2)

            cv2.imshow("Data Collection", frame)

            if collecting:
                filepath = gesture_dir / f"{idx:04d}.jpg"
                cv2.imwrite(str(filepath), frame)
                idx += 1
                time.sleep(args.interval)

            key = cv2.waitKey(1) & 0xFF
            if key == ord(' '):
                collecting = not collecting
                if collecting:
                    print(f"[INFO] 开始采集...")
                else:
                    print(f"[INFO] 暂停，已采集 {idx - start_idx} 张")
            elif key == ord('q'):
                break

    print(f"[INFO] 完成！共采集 {idx - start_idx} 张图片")
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
