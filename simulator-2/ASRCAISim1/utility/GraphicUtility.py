#-*-coding:utf-8-*-
'''最低限の可視化用の、OpenGL関係の描画を楽に行うためのユーティリティ。
'''
from math import *
import pygame
from OpenGL.GL import *
from OpenGL.GLU import *
import numpy as np

class Draw2D:
	'''3Dと2Dを切り替えるためのクラス。デフォルトが3Dとして、
	Draw2D.begin()とDraw2D.end()で挟まれた範囲が、幅width,高さheightの矩形領域への2D描画として扱われる。
	'''
	@classmethod
	def setShape(cls,width,height):
		'''描画領域の指定
		'''
		cls.width=width
		cls.height=height
	@classmethod
	def convertPos(cls,pos):
		'''([0,width],[0,height])で与えた位置情報を、glVertex2Dで描画する際の範囲[-1,+1]に変換する。
		'''
		return (2.0*pos[0]/cls.width-1.0,2.0*pos[1]/cls.height-1.0)
	@classmethod
	def begin(cls):
		'''2D描画の開始
		'''
		glDisable(GL_DEPTH_TEST)
		cls.oldMatrixMode=glGetIntegerv(GL_MATRIX_MODE)
		glMatrixMode(GL_PROJECTION)
		glPushMatrix()
		glLoadIdentity()
		glMatrixMode(GL_MODELVIEW)
		glPushMatrix()
		glLoadIdentity()
	@classmethod
	def end(cls):
		'''2D描画の終了
		'''
		glPopMatrix()
		glMatrixMode(GL_PROJECTION)
		glPopMatrix()
		glMatrixMode(cls.oldMatrixMode)
		glEnable(GL_DEPTH_TEST)

def drawText3D(text,font,pos,color,bg,ignoreDepth=True):
	'''3D空間に文字列を描画する。

	Args:
		text (str): 描画したい文字列。
		font (pygame.font): 描画したい文字列のフォント。
		pos (3-dim array): 文字列を描画したい位置(3D)。
		color (4-dim array): RGBAで表現した文字色。8bitの4次元配列で与える。
		bg (4-dim array): RGBAで表現した背景色。8bitの4次元配列で与える。
		ignoreDepth (bool): OpenGLのデプスバッファを有効にするか否か。
	'''
	if(ignoreDepth):
		glDisable(GL_DEPTH_TEST)
	img=font.render(text,True, color,bg)
	textData = pygame.image.tostring(img, "RGBA", True)
	glRasterPos3d(*pos)
	glDrawPixels(img.get_width(), img.get_height(), GL_RGBA, GL_UNSIGNED_BYTE, textData)
	if(ignoreDepth):
		glEnable(GL_DEPTH_TEST)
def drawText2D(text,font,pos,color,bg=None):
	'''2D空間に文字列を描画する。

	Args:
		text (str): 描画したい文字列。
		font (pygame.font): 描画したい文字列のフォント。
		pos (2-dim array): 文字列を描画したい位置(2D)。
		color (4-dim array): RGBAで表現した文字色。8bitの4次元配列で与える。
		bg (4-dim array): RGBAで表現した背景色。8bitの4次元配列で与える。
	'''
	img=font.render(text,True, color,bg)
	textData = pygame.image.tostring(img, "RGBA", True)
	glRasterPos2d(*Draw2D.convertPos(pos))
	glDrawPixels(img.get_width(), img.get_height(), GL_RGBA, GL_UNSIGNED_BYTE, textData)
def drawCircle3D(_pos, _x,_y,_r, _n):
	'''3D空間に円(多角形)を描画する。

	_x,_yに単位ベクトルでないものを与えれば楕円にもなる。また、直交判定は行わない。

	Args:
		_pos (3-dim array): 中心の座標
		_x (3-dim array): 円のx軸方向ベクトル
		_y (3-dim array): 円のy軸方向ベクトル
		_r (float): 半径
		_n (int): 分割数。
	'''
	y = np.cross(np.cross(_x,_y),_x)
	glBegin(GL_LINE_LOOP)
	for i in range(_n):
		tmp = _pos + _x*(_r*cos(2 * pi*i / _n)) + _y*(_r*sin(2 * pi*i / _n))
		glVertex3d(tmp[0], tmp[1], tmp[2])
	glEnd()
def drawCircle2D(_pos,_r,_n):
	'''2D空間に円(多角形)を描画する。

	Args:
		_pos (3-dim array): 中心の座標
		_r (float): 半径
		_n (int): 分割数。
	'''
	glBegin(GL_LINE_LOOP)
	for i in range(_n):
		tmp = Draw2D.convertPos(_pos + _r*np.array([cos(2 * pi*i / _n) , sin(2 * pi*i / _n)]))
		glVertex2d(tmp[0], tmp[1])
	glEnd()
def fillCircle3D(_pos, _x,_y,_r, _n):
	'''3D空間に塗りつぶし円(多角形)を描画する。

	_x,_yに単位ベクトルでないものを与えれば楕円にもなる。また、直交判定は行わない。

	Args:
		_pos (3-dim array): 中心の座標
		_x (3-dim array): 円のx軸方向ベクトル
		_y (3-dim array): 円のy軸方向ベクトル
		_r (float): 半径
		_n (int): 分割数。
	'''
	y = np.cross(np.cross(_x,_y),_x)
	glBegin(GL_TRIANGLE_FAN)
	tmp=_pos
	glVertex3d(tmp[0], tmp[1], tmp[2])
	for i in range(_n+1):
		tmp = _pos + _x*(_r*cos(2 * pi*i / _n)) + _y*(_r*sin(2 * pi*i / _n))
		glVertex3d(tmp[0], tmp[1], tmp[2])
	glEnd()
def fillCircle2D(_pos,_r,_n):
	'''2D空間に塗りつぶし円(多角形)を描画する。

	Args:
		_pos (3-dim array): 中心の座標
		_r (float): 半径
		_n (int): 分割数。
	'''
	glBegin(GL_TRIANGLE_FAN)
	tmp = Draw2D.convertPos(_pos)
	glVertex2d(tmp[0],tmp[1])
	for i in range(_n+1):
		tmp = Draw2D.convertPos(_pos + _r*np.array([cos(2 * pi*i / _n) , sin(2 * pi*i / _n)]))
		glVertex2d(tmp[0], tmp[1])
	glEnd()
def drawFan2D(_pos,_r,_t0,_t1,_n):
	'''2D空間に扇形(多角形)を描画する。

	Args:
		_pos (3-dim array): 中心の座標
		_r (float): 半径
		_t0 (float): 始点の偏角[rad]
		_t1 (float): 終点の偏角[rad]
		_n (int): 分割数。
	'''
	glBegin(GL_LINE_LOOP)
	tmp=Draw2D.convertPos(_pos)
	glVertex2d(tmp[0], tmp[1])
	for i in range(_n):
		tmp = Draw2D.convertPos(_pos + _r*np.array([cos(_t0+(_t1-_t0)*i/ _n) , sin(_t0+(_t1-_t0)*i/ _n)]))
		glVertex2d(tmp[0], tmp[1])
	glEnd()
def fillFan2D(_pos,_r,_t0,_t1,_n):
	'''2D空間に塗りつぶし扇形(多角形)を描画する。

	Args:
		_pos (3-dim array): 中心の座標
		_r (float): 半径
		_t0 (float): 始点の偏角[rad]
		_t1 (float): 終点の偏角[rad]
		_n (int): 分割数。
	'''
	glBegin(GL_TRIANGLE_FAN)
	tmp = Draw2D.convertPos(_pos)
	glVertex2d(tmp[0],tmp[1])
	for i in range(_n+1):
		tmp = Draw2D.convertPos(_pos + _r*np.array([cos(_t0+(_t1-_t0)*i/ _n) , sin(_t0+(_t1-_t0)*i/ _n)]))
		glVertex2d(tmp[0], tmp[1])
	glEnd()
def drawRect2D(_x,_y,_w,_h):
	'''2D空間に矩形を描画する。

	Args:
		_x (float): x方向の最小値
		_y (float): y方向の最小値
		_w (float): x方向の幅
		_h (float): y方向の高さ
	'''
	glBegin(GL_LINE_LOOP)
	glVertex2d(*Draw2D.convertPos((_x,_y)))
	glVertex2d(*Draw2D.convertPos((_x,_y+_h)))
	glVertex2d(*Draw2D.convertPos((_x+_w,_y+_h)))
	glVertex2d(*Draw2D.convertPos((_x+_w,_y)))
	glEnd()
def fillRect2D(_x,_y,_w,_h):
	'''2D空間に塗りつぶし矩形を描画する。

	Args:
		_x (float): x方向の最小値
		_y (float): y方向の最小値
		_w (float): x方向の幅
		_h (float): y方向の高さ
	'''
	glBegin(GL_QUADS)
	glVertex2d(*Draw2D.convertPos((_x,_y)))
	glVertex2d(*Draw2D.convertPos((_x,_y+_h)))
	glVertex2d(*Draw2D.convertPos((_x+_w,_y+_h)))
	glVertex2d(*Draw2D.convertPos((_x+_w,_y)))
	glEnd()
def drawLine2D(x1,y1,x2,y2):
	'''2D空間に線分を描画する。

	Args:
		x1 (float): 始点のx座標
		y1 (float): 始点のy座標
		x2 (float): 終点のx座標
		y2 (float): 終点のy座標
	'''
	glBegin(GL_LINES)
	glVertex2d(*Draw2D.convertPos((x1,y1)))
	glVertex2d(*Draw2D.convertPos((x2,y2)))
	glEnd()
def drawPipe(_r0,_r1,_radius):
	'''3D空間に円柱を描画する。

	Args:
		_r0 (3-dim array): 始点の座標
		_r1 (3-dim array): 終点の座標
		_radius (float): 半径
	'''
	r2=r1-r0
	L=np.linalg.norm(r2)
	ang=acos(r2[2]/L)/pi*180.0
	arrow=gluNewQuadric()
	glPushMatrix()
	glTranslated(_r0[0], _r0[1], _r0[2])
	glRotated(ang, -r2[1]*L, r2[0]*L, 0.0)
	arrow = gluNewQuadric()
	gluQuadricDrawStyle(arrow, GLU_FILL)
	gluCylinder(arrow, _radius, _radius, len, 8, 1)
	glPopMatrix()
	gluDeleteQuadric(arrow)

def polygonRegionCut(ps,reg,axis):
	'''(仮実装)凸多角形をx軸またはy軸の指定範囲の境界で切断し、指定範囲内に収まる新たな多角形として返す。

	pygameのウィンドウで複数のGLサーフェスを使えないため、
	2種類以上の画面を並べる際に、隣の画面にはみ出さないようにするための処置。
	例えばmodernGLを導入してFBOを使うなどすれば解決するので将来的には不要になる。
	
	Args:
		ps (list of 2-dim array): 多角形を構成する点の2D座標のリスト。
		reg (2-dim array): 収めたい範囲。[最小値,最大値]
		axis (int): 切断したい軸。x軸のときは0、y軸のときは1を入れる。
	'''
	#axis軸をreg[0],reg[1]に収まるように凸多角形を区切る
	inRange=[reg[0]<=p[axis] and p[axis]<=reg[1] for p in ps]
	ret=ps
	if(inRange[0]):#1つめが内側
		intercept=[-1]
		mid=[]
		for i in range(1,len(ps)):
			if(len(intercept)%2==1 and not inRange[i]):
				inner=ps[i-1]
				outer=ps[i]
				intercept[-1]=i
				intercept.append(-1)
				dst=reg[1] if outer[axis]>=reg[1] else reg[0]
				mid.append(inner+(outer-inner)*((dst-inner[axis])/(outer[axis]-inner[axis])))
			if(len(intercept)%2==0 and inRange[i]):
				outer=ps[i-1]
				inner=ps[i]
				dst=reg[1] if outer[axis]>=reg[1] else reg[0]
				mid.append(outer+(inner-outer)*((dst-outer[axis])/(inner[axis]-outer[axis])))
				intercept[-1]=i
				intercept.append(-1)
		if(len(intercept)%2==0):
			outer=ps[-1]
			inner=ps[0]
			dst=reg[1] if outer[axis]>=reg[1] else reg[0]
			mid.append(outer+(inner-outer)*((dst-outer[axis])/(inner[axis]-outer[axis])))
			intercept[-1]=0
		ret=[]
		bef=0
		aft=-1
		for i in range(int(len(mid)/2)):
			aft=intercept[i*2]
			ret=ret+ps[bef:aft]+mid[i*2:i*2+2]
			bef=intercept[i*2+1]
		if(len(mid)==0 or bef>0):
			ret=ret+ps[bef:]
	return ret
