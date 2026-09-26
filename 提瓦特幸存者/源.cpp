#include <graphics.h>   // 导入图形库 <graphics.h> 用于图形界面的绘制和事件处理
#include <string>		// 导入标准字符串库 <string> 以使用字符串操作功能
#include <vector>		// 导入标准模板库中的 <vector> 用于操作动态数组

const int PLAYER_ANIM_NUM = 6; // 定义玩家动画帧的数量为 6

const int WINDOW_WIDTH = 1280; // 定义游戏窗口的宽度为 1280 像素
const int WINDOW_HEIGHT = 720; // 定义游戏窗口的高度为 720 像素

const int BUTTON_WIDTH = 192; // 定义按钮的宽度为 192 像素
const int BUTTON_HEIGHT = 75; // 定义按钮的高度为 75 像素

#pragma comment(lib, "Winmm.lib")		// 添加编译器指令以自动链接到 Winmm.lib，用于多媒体功能，如播放背景音乐
#pragma comment(lib, "MSIMG32.LIB")		// 添加编译器指令以自动链接到 MSIMG32.LIB，提供图像绘制功能，如透明度混合

bool running = true;			// 定义一个全局变量 running 来控制游戏主循环是否继续进行
bool is_game_started = false;	// 定义一个全局变量 is_game_started 来标示游戏是否已经开始
// 定义一个内联函数 putimage_alpha 用来在屏幕上绘制带有透明度的图像
// x, y 是将图像绘制在屏幕上的左上角坐标
// img 是指向要绘制的图像对象的指针
inline void putimage_alpha(int x, int y, IMAGE* img)
{
	int w = img->getwidth();					// 获取图像的宽度 w
	int h = img->getheight();					// 获取图像的高度 h
	AlphaBlend(GetImageHDC(NULL), x, y, w, h,	// 使用 AlphaBlend 函数在屏幕上绘制具有一定透明度的图像
		GetImageHDC(img), 0, 0, w, h, { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA });
}
// 定义一个类 Atlas 来表示一个动画的集合，使用 std::vector 来存储图像帧列表
class Atlas
{
public:// 下面定义的方法和成员是公共的，即外部代码可以访问
	Atlas(LPCTSTR path, int num)// 构造函数，接收图像路径模版和帧数，按序号加载动画的每一帧
	{
		TCHAR path_file[256]; // 定义一个足够大的字符数组来存储完整文件路径
		for (size_t i = 0; i < num; i++) // 循环 num 次，加载所有动画帧
		{
			// 根据序号和模板格式化图像的完整路径
			_stprintf_s(path_file, path, i);

			// 使用格式化好的路径创建图像帧，使用 new 分配动态内存
			IMAGE* frame = new IMAGE();
			// 加载图像到创建好的 IMAGE 对象中
			loadimage(frame, path_file);
			// 将加载好的图像帧添加到动画帧列表中
			frame_list.push_back(frame);
		}
	}

	// 析构函数，在 Atlas 类对象销毁时调用，用来释放资源
	~Atlas()
	{
		for (size_t i = 0; i < frame_list.size(); i++) // 循环遍历动画帧列表
			delete frame_list[i]; // 使用 delete 释放每一帧动画的内存
	}

public: // 公共成员
	// 动画帧列表，用于存储动画中的每一帧图像
	std::vector<IMAGE*> frame_list;
};

// 全局变量，存储各种动作的动画帧集合
Atlas* atlas_player_left;	 // 玩家向左走动画帧集合
Atlas* atlas_player_right;   // 玩家向右走动画帧集合
Atlas* atlas_enemy_left;	 // 敌人向左走动画帧集合
Atlas* atlas_enemy_right;	 // 敌人向右走动画帧集合

class Animation
{
public:
	// 构造函数，用于初始化动画对象
	Animation(Atlas* atlas, int interval)
	{
		anim_atlas = atlas; // 动画帧集
		interval_ms = interval; // 帧间隔时间（以毫秒为单位）
	}

	// 默认析构函数
	~Animation() = default;

	// 根据时间差delta更新和播放动画
	void Play(int x, int y, int delta)
	{
		timer += delta; // 累加时间以便于确定何时切换动画帧
		// 如果累加时间到达设定的间隔时刻，切换到下一帧
		if (timer >= interval_ms)
		{
			idx_frame = (idx_frame + 1) % anim_atlas->frame_list.size(); // 循环播放动画帧
			timer = 0; // 重置计时器用于下一次帧切换计时
		}
		// 绘制当前动画帧到(x, y)位置，带有alpha通道
		putimage_alpha(x, y, anim_atlas->frame_list[idx_frame]);
	}

private:
	int timer = 0; // 计时器，用于控制动画帧切换的时间
	int idx_frame = 0; // 动画当前帧索引
	int interval_ms = 0; // 帧切换间隔时间（毫秒）
private:
	Atlas* anim_atlas; // 动画帧集，包含所有动画帧
};

class Player
{
public:
	// 玩家帧宽度和高度的常量定义
	const int FRAME_WIDTH = 80;
	const int FRAME_HEIGHT = 80;

public:
	// 构造函数，初始化玩家所需的资源
	Player()
	{
		// 加载玩家阴影图像
		loadimage(&img_shadow, _T("img/shadow_player.png"));
		// 创建向左动画和向右动画
		anim_left = new Animation(atlas_player_left, 45);
		anim_right = new Animation(atlas_player_right, 45);
	}

	// 析构函数，释放动画资源
	~Player()
	{
		delete anim_left; // 释放向左动画资源
		delete anim_right; // 释放向右动画资源
	}

	// 处理玩家输入事件
	void ProcessEvent(const ExMessage& msg)
	{
		// 分别对不同的Windows消息做出处理
		switch (msg.message)
		{
			// 键盘按键按下事件
		case WM_KEYDOWN:
			// 根据按键代码设置移动标志
			switch (msg.vkcode)
			{
			case VK_UP:
				is_move_up = true;
				break;
			case VK_DOWN:
				is_move_down = true;
				break;
			case VK_LEFT:
				is_move_left = true;
				break;
			case VK_RIGHT:
				is_move_right = true;
				break;
			}
			break;
			// 键盘按键释放事件
		case WM_KEYUP:
			// 根据按键代码清除移动标志
			switch (msg.vkcode)
			{
			case VK_UP:
				is_move_up = false;
				break;
			case VK_DOWN:
				is_move_down = false;
				break;
			case VK_LEFT:
				is_move_left = false;
				break;
			case VK_RIGHT:
				is_move_right = false;
				break;
			}
			break;
		}
	}

	// 根据输入事件更新玩家的位置
	void Move()
	{
		// 计算玩家的移动方向
		int dir_x = is_move_right - is_move_left; // 水平方向：右为正，左为负
		int dir_y = is_move_down - is_move_up; // 垂直方向：下为正，上为负
		// 计算移动向量的模长（长度）
		double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);
		// 当模长不为0时，说明有移动发生
		if (len_dir != 0)
		{
			// 计算单位向量，用于保持移动速度恒定
			double normalized_x = dir_x / len_dir;
			double normalized_y = dir_y / len_dir;
			// 更新玩家位置
			position.x += (int)(SPEED * normalized_x);
			position.y += (int)(SPEED * normalized_y);
		}
		// 限制玩家的移动范围，使其不能移出屏幕
		if (position.x < 0) position.x = 0;
		if (position.y < 0) position.y = 0;
		if (position.x + FRAME_WIDTH > WINDOW_WIDTH) position.x = WINDOW_WIDTH - FRAME_WIDTH;
		if (position.y + FRAME_HEIGHT > WINDOW_HEIGHT) position.y = WINDOW_HEIGHT - FRAME_HEIGHT;
	}

	// 根据玩家的位置和朝向绘制玩家图像
	void Draw(int delta)
	{
		// 计算阴影的位置，确保它位于玩家底部且居中
		int pos_shadow_x = position.x + (FRAME_WIDTH / 2 - SHADOW_WIDTH / 2);
		int pos_shadow_y = position.y + FRAME_HEIGHT - 8; // 阴影上移一些以更自然地显示
		// 绘制阴影
		putimage_alpha(pos_shadow_x, pos_shadow_y, &img_shadow);

		// 面向方向的标志，用来决定使用哪个动画
		static bool facing_left = false;
		int dir_x = is_move_right - is_move_left;
		// 更新玩家的朝向
		if (dir_x < 0)
			facing_left = true;
		else if (dir_x > 0)
			facing_left = false;

		// 根据朝向选择和播放相应的动画
		if (facing_left)
			anim_left->Play(position.x, position.y, delta);
		else
			anim_right->Play(position.x, position.y, delta);
	}

	// 获取玩家的当前位置
	const POINT& GetPosition() const
	{
		return position;
	}

private:
	// 玩家的移动速度
	const int SPEED = 3;
	// 阴影的宽度
	const int SHADOW_WIDTH = 32;

private:
	// 玩家的阴影图像
	IMAGE img_shadow;
	// 玩家向左移动时的动画对象指针
	Animation* anim_left;
	// 玩家向右移动时的动画对象指针
	Animation* anim_right;
	// 玩家当前的位置
	POINT position = { 500, 500 };
	// 各个方向移动的标志
	bool is_move_up = false;
	bool is_move_down = false;
	bool is_move_left = false;
	bool is_move_right = false;
};

// Button类：表示屏幕上的按钮，负责处理与按钮相关的用户交互
class Button
{
public:// 公有部分：对外公开的接口和方法
	// 构造函数：创建一个新的按钮实例
	Button(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		: region(rect) // 初始化成员region，用于设定按钮在屏幕上的位置和尺寸
	{
		// 为按钮的三种状态（静态、悬停、按下）加载不同的图像
		loadimage(&img_idle, path_img_idle); // 加载按钮未被悬停或点击时的静态图像
		loadimage(&img_hovered, path_img_hovered); // 加载鼠标悬停在按钮上时的图像
		loadimage(&img_pushed, path_img_pushed); // 加载按钮被点击时的图像
	}

	// 析构函数：负责清理Button类实例，释放内存资源
	~Button() = default;

	// ProcessEvent函数：处理鼠标事件，并改变按钮的视觉状态
	void ProcessEvent(const ExMessage& msg)
	{
		// 根据鼠标事件类型来更新按钮的状态
		switch (msg.message)
		{
		case WM_MOUSEMOVE: // 鼠标移动消息
			if (status == Status::Idle && CheckCursorHit(msg.x, msg.y))
				status = Status::Hovered; // 如果鼠标移入按钮区域，修改状态为悬停
			else if (status == Status::Hovered && !CheckCursorHit(msg.x, msg.y))
				status = Status::Idle; // 如果鼠标移出按钮区域，修改状态为静态
			break;
		case WM_LBUTTONDOWN: // 鼠标左键按下消息
			if (CheckCursorHit(msg.x, msg.y))
				status = Status::Pushed; // 如果在按钮区域按下鼠标左键，修改状态为按下
			break;
		case WM_LBUTTONUP: // 鼠标左键释放消息
			if (status == Status::Pushed)
				onClick(); // 如果鼠标左键在按下状态释放，调用onClick函数
			break;
		default: // 默认消息，不处理
			break;
		}
	}

	// Draw函数：根据按钮当前的状态绘制对应的按钮图像
	void Draw()
	{
		// 根据按钮的当前状态选择要绘制的图像
		switch (status)
		{
		case Status::Idle:
			putimage(region.left, region.top, &img_idle); // 绘制静态图像
			break;
		case Status::Hovered:
			putimage(region.left, region.top, &img_hovered); // 绘制悬停图像
			break;
		case Status::Pushed:
			putimage(region.left, region.top, &img_pushed); // 绘制按下图像
			break;
		}
	}

protected:	// 受保护部分：只有本类和继承本类的子类能够访问
	// onClick函数：虚函数，子类负责实现按钮点击时的具体行为
	virtual void onClick() = 0; // 纯虚函数，子类必须实现这个方法

private:	// 私有部分：类的内部实现，外部不可访问
	// Status枚举类：定义按钮的可能状态（静态、悬停、按下）
	enum class Status
	{
		Idle = 0, // 静态，按钮无交互状态
		Hovered,  // 悬停，鼠标悬停在按钮上
		Pushed	  // 按下，按钮被按下
	};

private:// 私有部分：类的内部实现，外部不可访问
	RECT region; // 按钮位置和大小的矩形
	IMAGE img_idle; // 静态状态下的按钮图像
	IMAGE img_hovered; // 悬停状态下的按钮图像
	IMAGE img_pushed; // 按下状态下的按钮图像
	Status status = Status::Idle; // 当前的按钮状态，默认为静态

private:// 私有部分：类的内部实现，外部不可访问
	// CheckCursorHit函数：检查鼠标指针是否在按钮区域内
	bool CheckCursorHit(int x, int y)
	{
		// 如果鼠标在按钮的矩形区域内返回真，否则返回假
		return (x >= region.left) && (x <= region.right) && (y >= region.top) && (y <= region.bottom);
	}
};

// StartGameButton继承自Button类：表示开始游戏的按钮，负责处理开始游戏的操作
class StartGameButton : public Button
{
public:// 公有部分：对外公开的接口和方法
	// 构造函数：调用基类构造函数，初始化开始游戏按钮实例
	StartGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		: Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {}

	// 析构函数：负责清理StartGameButton实例
	~StartGameButton() = default;

protected:
	// onClick函数：覆写父类的虚函数，实现开始游戏按钮的点击行为
	void onClick()
	{
		is_game_started = true; // 设置游戏开始的标志为真
		// 播放背景音乐，开始循环播放
		mciSendString(_T("play bgm repeat from 0"), NULL, 0, NULL);
	}
};

// 退出游戏按钮类，继承自Button类
class QuitGameButton : public Button
{
public://公有
	// 构造函数：接收一个矩形区域和三张按钮状态下的图片路径
	QuitGameButton(RECT rect, LPCTSTR path_img_idle, LPCTSTR path_img_hovered, LPCTSTR path_img_pushed)
		: Button(rect, path_img_idle, path_img_hovered, path_img_pushed) {}
	~QuitGameButton() = default; // 默认析构函数

protected:
	// 当点击按钮时，会调用此函数，设置游戏运行标志为false
	void onClick()
	{
		running = false; // 停止游戏运行
	}
};

// 子弹类，用于表示游戏中的子弹实体
class Bullet
{
public:
	POINT position = { 0, 0 }; // 子弹的位置

public:
	Bullet() = default; // 默认构造函数
	~Bullet() = default; // 默认析构函数

	// 绘制子弹的函数
	void Draw() const
	{
		setlinecolor(RGB(255, 155, 50)); // 设置描边颜色
		setfillcolor(RGB(200, 75, 10));  // 设置填充颜色
		fillcircle(position.x, position.y, RADIUS); // 以子弹位置为圆心，绘制一个圆形表示子弹
	}

private:
	const int RADIUS = 10; // 子弹半径
};

// 敌人类，用于表示游戏中的敌人实体
class Enemy
{
public:
	// 敌人的构造函数
	Enemy()
	{
		// 加载敌人的阴影图片
		loadimage(&img_shadow, _T("img/shadow_enemy.png"));
		// 创建左右移动的动画对象
		anim_left = new Animation(atlas_enemy_left, 45);
		anim_right = new Animation(atlas_enemy_right, 45);

		// 枚举敌人可能出现的边界
		enum class SpawnEdge
		{
			Up = 0,    // 上边界
			Down,      // 下边界
			Left,      // 左边界
			Right      // 右边界
		};
		// 随机选择一个边界让敌人出现
		SpawnEdge edge = (SpawnEdge)(rand() % 4);
		switch (edge)
		{
		case SpawnEdge::Up: // 敌人从上边界出现
			position.x = rand() % WINDOW_WIDTH;
			position.y = -FRAME_HEIGHT;
			break;
		case SpawnEdge::Down: // 敌人从下边界出现
			position.x = rand() % WINDOW_WIDTH;
			position.y = WINDOW_HEIGHT;
			break;
		case SpawnEdge::Left: // 敌人从左边界出现
			position.x = -FRAME_WIDTH;
			position.y = rand() % WINDOW_HEIGHT;
			break;
		case SpawnEdge::Right: // 敌人从右边界出现
			position.x = WINDOW_WIDTH;
			position.y = rand() % WINDOW_HEIGHT;
			break;
		default:
			break;
		}
	}

	// 此函数用于检测子弹是否与敌人发生碰撞。
	bool CheckBulletCollision(const Bullet& bullet)
	{
		// 检查子弹的位置是否与敌人的矩形框相交。首先检查X轴是否重叠。
		bool is_overlap_x = bullet.position.x >= position.x && bullet.position.x <= position.x + FRAME_WIDTH;
		// 接着检查Y轴是否重叠。
		bool is_overlap_y = bullet.position.y >= position.y && bullet.position.y <= position.y + FRAME_HEIGHT;
		// 如果X轴和Y轴都有重叠，则意味着发生了碰撞。
		return is_overlap_x && is_overlap_y;
	}
	// 此函数用于检测敌人是否与玩家发生碰撞。
	bool CheckPlayerCollision(const Player& player)
	{
		// 获取敌人中心点的位置。
		POINT check_position = { position.x + FRAME_WIDTH / 2, position.y + FRAME_HEIGHT / 2 };
		// 获取玩家位置。
		const POINT& player_position = player.GetPosition();
		// 检查敌人中心点是否在玩家的矩形框内。
		bool is_inside_x = (check_position.x >= player_position.x) && (check_position.x <= (player_position.x + FRAME_WIDTH));
		bool is_inside_y = (check_position.y >= player_position.y) && (check_position.y <= (player_position.y + FRAME_HEIGHT));
		// 如果中心点在玩家矩形框内的X轴和Y轴上，则发生碰撞。
		return is_inside_x && is_inside_y;
	}

	// 此函数表示敌人向玩家所在位置移动。
	void Move(const Player& player)
	{
		// 获取玩家的当前位置。
		const POINT& player_position = player.GetPosition();
		// 计算敌人位置与玩家位置的差距向量。
		int dir_x = player_position.x - position.x;
		int dir_y = player_position.y - position.y;
		// 计算这个向量的长度，用于后续的单位向量计算。
		double len_dir = sqrt(dir_x * dir_x + dir_y * dir_y);
		// 如果长度不为零（即敌人和玩家不在同一个位置），则进行单位向量计算。
		if (len_dir != 0)
		{
			double normalized_x = dir_x / len_dir;
			double normalized_y = dir_y / len_dir;
			// 使用单位向量更新敌人位置，SPEED为敌人速度常数。
			position.x += (int)(SPEED * normalized_x);
			position.y += (int)(SPEED * normalized_y);
		}
		// 根据敌人的移动方向更新面朝方向。
		if (dir_x < 0)
			facing_left = true;
		else if (dir_x > 0)
			facing_left = false;
	}

	// 绘制敌人在屏幕上的图像。
	void Draw(int delta)
	{
		// 计算阴影的坐标位置。
		int pos_shadow_x = position.x + (FRAME_WIDTH / 2 - SHADOW_WIDTH / 2);
		int pos_shadow_y = position.y + FRAME_HEIGHT - 8;
		// 绘制阴影图片。
		putimage_alpha(pos_shadow_x, pos_shadow_y, &img_shadow);
		// 根据敌人的面朝方向，选择对应的动画进行播放。
		if (facing_left)
			anim_left->Play(position.x, position.y, delta);
		else
			anim_right->Play(position.x, position.y, delta);
	}

	~Enemy()
	{// 析构函数中释放动画对象资源，避免内存泄漏。
		delete anim_left;
		delete anim_right;
	}
	void Hurt()// 此函数用于标记敌人为受伤状态，受伤后的敌人不再显示在屏幕上。
	{
		alive = false;
	}

	// 此函数返回敌人是否仍然存活。
	bool CheckAlive()
	{
		return alive;
	}

private:
	const int SPEED = 2;			// 敌人移动速度的常量。
	const int FRAME_WIDTH = 80;		// 敌人宽度
	const int FRAME_HEIGHT = 80;	// 敌人高度
	const int SHADOW_WIDTH = 48;	// 阴影宽度

private:
	IMAGE img_shadow;				// 用于存储敌人阴影的图像对象
	Animation* anim_left;			// 指针变量，指向代表敌人向左移动的动画对象。
	Animation* anim_right;			// 指针变量，指向代表敌人向右移动的动画对象。
	POINT position = { 0, 0 };		 // 代表敌人当前位置的点结构体。
	bool facing_left = false;		// 表示敌人当前是否面朝左边的布尔值。
	bool alive = true;				// 表示敌人是否存活的布尔值。
};

// 生成新的敌人
void TryGenerateEnemy(std::vector<Enemy*>& enemy_list)
{
	// 定义生成敌人的间隔时间。
	const int INTERVAL = 100;

	// 静态变量，用于计时。
	static int counter = 0;

	// 每当计时器达到间隔时间，就生成一个新的敌人。
	if ((++counter) % INTERVAL == 0)
		enemy_list.push_back(new Enemy());
}

// 更新子弹位置的函数，根据弧度和玩家位置来更新子弹的位置产生旋转动画。
void UpdateBullets(std::vector<Bullet>& bullet_list, const Player& player)
{
	// 定义径向波动速度和切向波动速度。
	const double RADIAL_SPEED = 0.0045;
	const double TANGENT_SPEED = 0.0055;

	// 计算子弹之间的弧度间隔。
	double radian_interval = 2 * 3.14159 / bullet_list.size();

	// 获取玩家当前位置。
	POINT player_position = player.GetPosition();

	// 计算波动的半径。
	double radius = 100 + 25 * sin(GetTickCount() * RADIAL_SPEED);

	// 遍历子弹列表，更新每个子弹的位置。
	for (size_t i = 0; i < bullet_list.size(); i++)
	{
		// 计算每个子弹对应的弧度值。
		double radian = GetTickCount() * TANGENT_SPEED + radian_interval * i;

		// 根据弧度值和半径计算子弹的位置。
		bullet_list[i].position.x = player_position.x + player.FRAME_WIDTH / 2 + (int)(radius * sin(radian));
		bullet_list[i].position.y = player_position.y + player.FRAME_HEIGHT / 2 + (int)(radius * cos(radian));
	}
}

// 绘制玩家当前得分
void DrawPlayerScore(int score)
{
	// 创建一个文本缓冲区。
	static TCHAR text[64];

	// 格式化字符串，将得分写入缓冲区。
	_stprintf_s(text, _T("当前玩家得分：%d"), score);

	// 设置背景透明。
	setbkmode(TRANSPARENT);

	// 设置文本颜色。
	settextcolor(RGB(255, 85, 185));

	// 在屏幕上绘制文本。
	outtextxy(10, 10, text);
}

int main()
{
	// 初始化绘图窗口。
	initgraph(1280, 720);

	// 加载玩家和敌人的动画资源。
	atlas_player_left = new Atlas(_T("img/player_left_%d.png"), 6);
	atlas_player_right = new Atlas(_T("img/player_right_%d.png"), 6);
	atlas_enemy_left = new Atlas(_T("img/enemy_left_%d.png"), 6);
	atlas_enemy_right = new Atlas(_T("img/enemy_right_%d.png"), 6);

	// 加载音效和背景音乐。
	mciSendString(_T("open mus/hit.wav alias hit"), NULL, 0, NULL);
	mciSendString(_T("open mus/bgm.mp3 alias bgm"), NULL, 0, NULL);

	// 初始化变量。
	int score = 0;
	Player player;
	ExMessage msg;
	IMAGE img_menu, img_background;
	std::vector<Enemy*> enemy_list;
	std::vector<Bullet> bullet_list(3);

	// 定义开始游戏按钮和退出游戏按钮的区域。
	RECT region_btn_start_game, region_btn_quit_game;

	region_btn_start_game.left = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
	region_btn_start_game.right = region_btn_start_game.left + BUTTON_WIDTH;
	region_btn_start_game.top = 430;
	region_btn_start_game.bottom = region_btn_start_game.top + BUTTON_HEIGHT;

	region_btn_quit_game.left = (WINDOW_WIDTH - BUTTON_WIDTH) / 2;
	region_btn_quit_game.right = region_btn_quit_game.left + BUTTON_WIDTH;
	region_btn_quit_game.top = 550;
	region_btn_quit_game.bottom = region_btn_quit_game.top + BUTTON_HEIGHT;

	// 创建开始游戏按钮和退出游戏按钮。
	StartGameButton btn_start_game = StartGameButton(region_btn_start_game,
		_T("img/ui_start_idle.png"), _T("img/ui_start_hovered.png"), _T("img/ui_start_pushed.png"));
	QuitGameButton btn_quit_game = QuitGameButton(region_btn_quit_game,
		_T("img/ui_quit_idle.png"), _T("img/ui_quit_hovered.png"), _T("img/ui_quit_pushed.png"));

	// 加载菜单图片和背景图片。
	loadimage(&img_menu, _T("img/menu.png"));
	loadimage(&img_background, _T("img/background.png"));

	// 开始批量绘制。
	BeginBatchDraw();

	// 游戏主循环。
	while (running)
	{
		// 获取当前时间。
		DWORD start_time = GetTickCount();

		// 处理消息。
		while (peekmessage(&msg))
		{
			if (is_game_started)
				player.ProcessEvent(msg);
			else
			{
				btn_start_game.ProcessEvent(msg);
				btn_quit_game.ProcessEvent(msg);
			}
		}

		// 如果游戏已经开始。
		if (is_game_started)
		{
			// 尝试生成新的敌人。
			TryGenerateEnemy(enemy_list);

			// 移动玩家。
			player.Move();

			// 更新子弹位置。
			UpdateBullets(bullet_list, player);

			// 更新敌人位置。
			for (Enemy* enemy : enemy_list)
				enemy->Move(player);

			// 检测子弹和敌人的碰撞。
			for (Enemy* enemy : enemy_list)
			{
				for (const Bullet& bullet : bullet_list)
				{
					if (enemy->CheckBulletCollision(bullet))
					{
						// 播放音效。
						mciSendString(_T("play hit from 0"), NULL, 0, NULL);

						// 标记敌人受伤。
						enemy->Hurt();

						// 增加玩家得分。
						score++;
					}
				}
			}

			// 移除生命值归零的敌人。
			for (size_t i = 0; i < enemy_list.size(); i++)
			{
				Enemy* enemy = enemy_list[i];
				if (!enemy->CheckAlive())
				{
					// 将敌人移动到列表末尾。
					std::swap(enemy_list[i], enemy_list.back());

					// 从列表中删除敌人。
					enemy_list.pop_back();

					// 释放敌人内存。
					delete enemy;
				}
			}

			// 检测敌人和玩家的碰撞。
			for (Enemy* enemy : enemy_list)
			{
				if (enemy->CheckPlayerCollision(player))
				{
					// 游戏结束，显示得分。
					static TCHAR text[128];
					_stprintf_s(text, _T("最终得分：%d !"), score);
					MessageBox(GetHWnd(), text, _T("游戏结束"), MB_OK);
					running = false;
					break;
				}
			}
		}

		// 清除屏幕。
		cleardevice();

		// 绘制游戏画面。
		if (is_game_started)
		{
			// 绘制背景。
			putimage(0, 0, &img_background);

			// 绘制玩家。
			player.Draw(1000 / 144);

			// 绘制敌人。
			for (Enemy* enemy : enemy_list)
				enemy->Draw(1000 / 144);
			// 绘制子弹。
			for (const Bullet& bullet : bullet_list)
				bullet.Draw();
			// 绘制玩家得分。
			DrawPlayerScore(score);
		}
		else
		{
			// 绘制菜单界面。
			putimage(0, 0, &img_menu);

			// 绘制开始游戏按钮。
			btn_start_game.Draw();

			// 绘制退出游戏按钮。
			btn_quit_game.Draw();
		}
		// 结束批量绘制。
		FlushBatchDraw();

		DWORD end_time = GetTickCount();
		DWORD delta_time = end_time - start_time;
		if (delta_time < 1000 / 144)
		{
			Sleep(1000 / 144 - delta_time);
		}
	}
	// 释放内存。
	delete atlas_player_left;
	delete atlas_player_right;
	delete atlas_enemy_left;
	delete atlas_enemy_right;

	// 结束批量绘制。
	EndBatchDraw();

	return 0;// 返回 0 表示程序成功退出。
}
