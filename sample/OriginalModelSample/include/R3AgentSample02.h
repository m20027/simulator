#pragma once
#include <ASRCAISim1/Agent.h>
#include <ASRCAISim1/Sensor.h>

DECLARE_CLASS_WITHOUT_TRAMPOLINE(R3AgentSample02,Agent)
	/*編隊全体で1つのAgentを割り当てる、中央集権方式での行動判断モデルの実装例。
	時系列情報の活用については、RNNを使わずとも、キーフレームでの値を並列で吐き出せる形で実装している。
	もしRNNを使う場合は、キーフレームを指定せず、瞬時値をそのまま観測として出力すればよい。
	場外・墜落の回避については、学習に委ねず南北・高度方向の移動に制限をかけることで対処している。
	もし一時的に場外に出ることが勝利に繋がりうると思えば、制限をなくしてもよい。
	速度については、遅くなりすぎると機動力が低下するため、下限速度を設定できるようにしている。
	射撃については、全弾連射する等ですぐに攻撃能力を喪失するような状況を予め回避するため、同時射撃数を制限できるようにしている。
	1. 観測データについて
		* 東側でも西側でも同じになるように、西側のベクトルはx,y軸について反転させる。
		* キーフレームは「n秒前のフレーム」として、そのnのlistをconfigで"pastPoints"キーにより与える
		* 各フレームの観測データの内訳
			1. 味方機情報・・・parentsに指定された味方機を、parentsに指定した順に最大maxTrackNum["Friend"]機分。生存していない場合は0埋め。
				1. 位置・・・x,y成分をRulerのdOutとdLineの大きい方で除し、z成分をRulerのhLimで除して正規化したもの。
				2. 速度・・・速度のノルムをfgtrVelNormalizerで正規化したものと、速度方向の単位ベクトルの4次元に分解したもの。
				3. 残弾数・・・intそのまま。（ただし一括してBoxの値として扱われる）
			2. 敵機情報・・・見えている敵機航跡のうち、近いものから順に最大maxTrackNum["Enemy"]機分。無い場合は0埋め。
				1. 位置・・・味方機と同じ
				2. 速度・・・味方機と同じ
			3. 味方誘導弾情報(自機も含む)・・・飛翔中の誘導弾のうち、射撃時刻が古いものから順に最大maxMissileNum["Friend"]発分。無い場合は0埋め。
				1. 位置・・・味方機と同じ
				2. 速度・・・味方機と同じ
				3. 誘導状態・・・guided,self,memoryの3通りについて、one-hot形式で与える。
			4. 敵誘導弾情報・・・見えている誘導弾航跡のうち、近いものから順に最大maxMissileNum["Enemy"]発分。無い場合は0埋め。
				1. 到来方向・・・慣性座標系での到来方向を表した単位ベクトル。
				2. 検出した味方機・・・int(非検出時を0とするため、1〜(1+maxTrackNum["Friend"]))
	2. 行動の形式について
		左右旋回、上昇・下降、加減速、射撃対象の4種類を離散化したものを全機分並べたものをMultiDiscreteで与える。
		1. 左右旋回・・・自機正面を0とした「行きたい方角(右を正)」で指定。
		2. 上昇・下降・・・水平を0とし、上昇・下降角度(下降を正)で指定。
		3. 加減速・・・目標速度を基準速度(≒現在速度)+ΔVとすることで表現し、ΔVで指定。
		4. 射撃対象・・・0を射撃なし、1〜maxTrackNumを対応するlastTrackInfoのTrackへの射撃とする。
	Attributes:
		* configで指定するもの
		turnScale (double): 最大限に左右旋回する際の目標方角の値。degで指定。
		turnDim (int): 左右旋回の離散化数。0を用意するために奇数での指定を推奨。
		pitchScale (double): 最大限に上昇・下降する際の目標角度の値。degで指定。
		pitchDim (int): 上昇・下降の離散化数。0を用意するために奇数での指定を推奨。
		accelScale (double): 最大限に加減速する際のΔVの絶対値。
		accelDim (int): 加減速の離散化数。0を用意するために奇数での指定を推奨。
		hMin (double): 高度下限。下限を下回ったら下回り具合に応じて上昇方向に針路を補正する。
		hMax (double): 高度上限。上限を上回ったら上回り具合に応じて下降方向に針路を補正する。
		dOutLimitRatio (double): 南北方向への戦域逸脱を回避するための閾値。中心からdOut×dOutLimitRatio以上外れたら外れ具合に応じて中心方向に針路を補正する。
		rangeNormalizer (float): 距離の正規化のための除数
		fgtrVelNormalizer (float): 機体速度の正規化のための除数
		mslVelNormalizer (float): 誘導弾速度の正規化のための除数
		maxTrackNum (dict): 観測データとして使用する味方、敵それぞれの航跡の最大数。{"Friend":3,"Enemy":4}のようにdictで指定する。
		maxMissileNum (dict): 観測データとして使用する味方、敵それぞれの誘導弾情報の最大数。書式はmaxTrackNumと同じ。
		pastPoints (list of int): キーフレームのリスト。「nステップ前のフレーム」としてnのリストで与える。空で与えれば瞬時値のみを使用する。RNNにも使える(はず)
		pastData (list of numpy.ndarray): 過去のフレームデータを入れておくリスト。キーフレームの間隔が空いていても、等間隔でなければ全フレーム分必要なので、全フレーム部分用意し、リングバッファとして使用する。
		minimumV (double): 下限速度。この値を下回ると指定した目標速度に戻るまで強制的に加速する。
		minimumRecoveryV (Double): 速度下限からの回復を終了する速度。下限速度に達した場合、この値に達するまで強制的に加速する。
		minimumRecoveryDstV (Double): 速度下限からの回復目標速度。下限速度に達した場合、この値を目標速度として加速する。
		maxSimulShot (int): 同時射撃数の制限。自身が発射した、飛翔中の誘導弾がこの数以下のときのみ射撃可能。
	*/
    public:
    //コンフィグ
    double turnScale,pitchScale,accelScale;
	int turnDim,pitchDim,accelDim;
	double hMin,hMax,dOutLimitRatio,rangeNormalizer,fgtrVelNormalizer,mslVelNormalizer;
    std::map<std::string,int> maxTrackNum,maxMissileNum;
    std::vector<int> pastPoints;
    std::vector<Eigen::VectorXf> pastData;
    double minimumV,minimumRecoveryV,minimumRecoveryDstV;
	int maxSimulShot;
    //内部変数
	int singleDim;
	VecX<long> lastActions;
    std::vector<Track3D> lastTrackInfo;
    std::map<std::string,Eigen::Vector3d> dstDir,omegaScale;
    std::map<std::string,bool> launchFlag,velRecovery;
	std::map<std::string,double> baseV,dstV;
    std::map<std::string,Track3D> target;
    Eigen::VectorXd turnTable,pitchTable,accelTable,fireTable;
    double dOut,dLine,hLim;
    Eigen::Vector3d xyInv;
    public:
    R3AgentSample02(const nl::json& modelConfig_,const nl::json& instanceConfig_);
    virtual ~R3AgentSample02();
    virtual void validate() override;
    virtual py::object observation_space() override;
    virtual py::object makeObs() override;
    Eigen::VectorXf makeSingleObs();
    virtual py::object action_space() override;
    virtual void deploy(py::object action) override;
    virtual void control() override;
    virtual py::object convertActionFromAnother(const nl::json& decision,const nl::json& command) override;
};

void exportR3AgentSample02(py::module &m);
