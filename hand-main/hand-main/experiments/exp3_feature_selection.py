"""
实验三: 特征选择实验
比较不同特征组合对分类性能的影响
"""

import sys
from pathlib import Path
import numpy as np
from sklearn.model_selection import train_test_split

sys.path.insert(0, str(Path(__file__).parent.parent))

from src.detection import HandDetector
from src.features import FeatureExtractor
from src.classifier import KNNClassifier
import cv2


def main():
    print("=" * 60)
    print("实验三: 特征选择实验")
    print("=" * 60)

    model_dir = Path("data/models")
    if not (model_dir / "knn_model.pkl").exists():
        print("[ERROR] 请先运行 scripts/train.py 训练模型")
        print("\n实验流程:")
        print("1. 加载数据集 → 分别用三种特征集提取特征")
        print("2. distance_only: 指尖距离 + 指尖-手腕距离 (15维)")
        print("3. angle_only:    手指关节弯曲角度 (10维)")
        print("4. fused:          距离+角度融合 (25维)")
        print("5. 训练三个KNN模型并对比准确率")
        print("6. 分析不同特征对分类的贡献")
        return

    print("[INFO] 实验框架就绪，使用 scripts/train.py --feature <type> 进行对比")
    print("\n运行方式:")
    print("  python scripts/train.py --feature distance  # 仅距离特征")
    print("  python scripts/train.py --feature angle     # 仅角度特征")
    print("  python scripts/train.py --feature all       # 融合特征")


if __name__ == "__main__":
    main()
