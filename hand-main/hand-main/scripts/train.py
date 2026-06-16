"""
模型训练脚本 - Training Script
从数据集图片提取特征，训练KNN分类器
"""

import sys
import time
import argparse
import pickle
from pathlib import Path

import cv2
import numpy as np
from sklearn.model_selection import train_test_split, cross_val_score
from sklearn.metrics import classification_report, confusion_matrix

sys.path.insert(0, str(Path(__file__).parent.parent))

from src.detection import HandDetector
from src.features import FeatureExtractor
from src.classifier import KNNClassifier


def parse_args():
    parser = argparse.ArgumentParser(description="训练KNN手势分类器")
    parser.add_argument("--data", type=str, default="data/raw",
                        help="原始数据集目录")
    parser.add_argument("--output", type=str, default="data/models/knn_model.pkl",
                        help="模型输出路径")
    parser.add_argument("--k", type=int, default=5, help="KNN的K值")
    parser.add_argument("--feature", type=str, default="all",
                        choices=["distance", "angle", "all"],
                        help="特征集类型")
    parser.add_argument("--test-size", type=float, default=0.2,
                        help="测试集比例")
    return parser.parse_args()


def load_dataset(data_dir: str):
    """
    加载数据集，对每张图片进行关键点检测和特征提取

    Returns:
        X: 特征矩阵
        y: 标签数组
        label_map: {目录名: 标签}
        failed: 检测失败的文件列表
    """
    data_path = Path(data_dir)
    gesture_dirs = sorted([d for d in data_path.iterdir() if d.is_dir()])

    print(f"[INFO] 发现 {len(gesture_dirs)} 个手势类别: "
          f"{[d.name for d in gesture_dirs]}")

    label_map = {d.name: i for i, d in enumerate(gesture_dirs)}
    print(f"[INFO] 标签映射: {label_map}")

    detector = HandDetector(static_image_mode=True, max_num_hands=1,
                            min_detection_confidence=0.5)
    extractor = FeatureExtractor()

    X_list = []
    y_list = []
    failed = []

    for gesture_dir in gesture_dirs:
        label = label_map[gesture_dir.name]
        image_files = list(gesture_dir.glob("*.jpg")) + \
                      list(gesture_dir.glob("*.png"))
        print(f"[INFO] 处理 '{gesture_dir.name}' ({len(image_files)} 张)...")

        success_count = 0
        for img_path in image_files:
            image = cv2.imread(str(img_path))
            if image is None:
                continue

            detected, landmarks = detector.detect(image)
            if not detected or landmarks is None:
                failed.append(str(img_path))
                continue

            try:
                features = extractor.extract(landmarks, args.feature)
                X_list.append(features)
                y_list.append(label)
                success_count += 1
            except Exception as e:
                failed.append(f"{img_path} ({e})")
                continue

        print(f"  -> 成功: {success_count}/{len(image_files)}")

    detector.close()

    X = np.array(X_list, dtype=np.float32)
    y = np.array(y_list, dtype=np.int32)

    print(f"\n[INFO] 数据集: {X.shape[0]} 样本, {X.shape[1]} 维特征")
    print(f"[INFO] 失败: {len(failed)} 张")

    return X, y, label_map, failed


def main():
    args = parse_args()

    # 1. 加载数据集
    print("=" * 50)
    print("加载数据集...")
    X, y, label_map, failed = load_dataset(args.data)

    if len(X) == 0:
        print("[ERROR] 无有效样本，请检查数据集")
        return

    # 2. 划分训练集/测试集
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=args.test_size, random_state=42, stratify=y
    )
    print(f"\n[INFO] 训练集: {X_train.shape[0]} 样本")
    print(f"[INFO] 测试集: {X_test.shape[0]} 样本")

    # 3. 训练模型
    print(f"\n训练 KNN (K={args.k}, feature={args.feature})...")
    start = time.time()
    classifier = KNNClassifier(k=args.k)
    classifier.fit(X_train, y_train)
    train_time = time.time() - start
    print(f"[INFO] 训练耗时: {train_time:.3f}s")

    # 4. 评估
    print("\n" + "=" * 50)
    print("模型评估")
    print("=" * 50)

    y_pred = classifier.predict(X_test)
    # 如果只有1个样本，需要reshape
    if len(y_pred.shape) == 0:
        y_pred = np.array([y_pred])

    label_names = [label_map.get(str(i), str(i)) for i in range(len(label_map))]
    actual_names = []
    for k, v in sorted(label_map.items(), key=lambda x: x[1]):
        actual_names.append(k)

    print(classification_report(y_test, y_pred,
                                target_names=actual_names,
                                zero_division=0))

    # 交叉验证
    print("\n交叉验证:")
    try:
        cv_scores = cross_val_score(classifier.classifier, classifier.scaler.transform(X),
                                    y, cv=5)
        print(f"  5折交叉验证准确率: {cv_scores.mean():.4f} (+/- {cv_scores.std():.4f})")
    except Exception as e:
        print(f"  交叉验证跳过: {e}")

    # 5. 测试不同K值
    print("\n" + "=" * 50)
    print("K值对比实验")
    print("=" * 50)
    print(f"{'K':<6} {'准确率':<10}")
    print("-" * 16)
    for k_test in [1, 3, 5, 7, 9, 11, 15]:
        clf = KNNClassifier(k=k_test)
        clf.fit(X_train, y_train)
        acc = clf.classifier.score(clf.scaler.transform(X_test), y_test)
        marker = " <--" if k_test == args.k else ""
        print(f"{k_test:<6} {acc:.4f}  {marker}")

    # 6. 保存模型
    Path(args.output).parent.mkdir(parents=True, exist_ok=True)
    classifier.save(args.output)
    print(f"\n[INFO] 模型已保存: {args.output}")

    # 保存标签映射
    label_path = Path(args.output).parent / "label_map.pkl"
    with open(label_path, 'wb') as f:
        pickle.dump(label_map, f)
    print(f"[INFO] 标签映射已保存: {label_path}")

    if failed:
        print(f"\n[WARN] {len(failed)} 张图片检测失败:")
        for f in failed[:10]:
            print(f"  - {f}")
        if len(failed) > 10:
            print(f"  ... 共 {len(failed)} 张")


if __name__ == "__main__":
    main()
