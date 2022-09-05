import sys
from ASRCAISim1.viewer.GodViewLoader import GodViewLoader

if __name__ == "__main__":
    if(len(sys.argv)>1):
        loader=GodViewLoader({
        "globPattern":sys.argv[1],
        "outputPrefix":sys.argv[2] if len(sys.argv)>2 else None,
        "asVideo":True, # Trueだと動画(mp4)、Falseだと連番画像（png）として保存
        "fps":60
        })
        loader.run()