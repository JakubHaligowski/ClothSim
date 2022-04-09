#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"


struct Point
{
	olc::vf2d postion, prevPostion;
	bool locked=false;	
};

struct Stick
{
	std::shared_ptr<Point> PointA;
	std::shared_ptr<Point> PointB;
	float lenght;

	Stick() {}
	
	Stick(std::shared_ptr<Point> A, std::shared_ptr<Point> B)
	{
		PointA = A;
		PointB = B;
		olc::vf2d pos1 = PointA->postion;
		olc::vf2d pos2 = PointB->postion;
		olc::vf2d temp = (PointA->postion) - (PointB->postion);
		lenght = temp.mag();
	}
};


class ClothSim : public olc::PixelGameEngine
{
public:
	float TargetFrameTime = 1.0f / 60.f;
	float AccumulatedTime = 0.0f;
	float bounce = 0.9f;
	float gravity = 0.5f;
	float friction = 0.999f;
	bool  doSim = false;

	std::shared_ptr<Point> selectedPoint;

	std::vector<std::shared_ptr<Point>> points;
	std::vector<Stick> sticks;
	
	void movePoints()
	{
		if (GetKey(olc::SHIFT).bHeld)
		{
			float x = GetMouseX();
			float y = GetMouseY();

			for (auto& p : points)
			{
				float distance2 = (x - p->postion.x) * (x - p->postion.x) + (y - p->postion.y) * (y - p->postion.y);
				if (distance2 < 25.0f)
				{
					p->postion.x = x;
					p->postion.y = y;
					p->prevPostion.x = x;
					p->prevPostion.y = y;
					return;
				}
			}
		}
	}

	void removePoint()//Bug when both poinst of a stick a remove the stick does not update
	{
		if (GetKey(olc::CTRL).bHeld)
		{
			float x = GetMouseX();
			float y = GetMouseY();

			auto it = points.begin();
			while (it!=points.end())
			{
				float distance2 = (x - (*it)->postion.x)*(x - (*it)->postion.x) + (y - (*it)->postion.y) * (y - (*it)->postion.y);
				if (distance2 < 25.0f)
				{
					points.erase(it);
					return;
				}
				it++;
			}
		}
	}

	void removeStick()
	{
		if (GetKey(olc::CTRL).bHeld)
		{
			float x = GetMouseX();
			float y = GetMouseY();

			auto it = sticks.begin();
			while (it != sticks.end())
			{
				float disA = sqrt((x - it->PointA->postion.x) * (x - it->PointA->postion.x) + (y - it->PointA->postion.y) * (y - it->PointA->postion.y));
				float disB = sqrt((x - it->PointB->postion.x) * (x - it->PointB->postion.x) + (y - it->PointB->postion.y) * (y - it->PointB->postion.y));
				float lenght = sqrt((it->PointA->postion.x - it->PointB->postion.x) * (it->PointA->postion.x - it->PointB->postion.x) + (it->PointA->postion.y - it->PointB->postion.y) * (it->PointA->postion.y - it->PointB->postion.y));
				float buffer = 1.7f;
				if (disA + disB > lenght - buffer && disA + disB < lenght + buffer)
				{
					sticks.erase(it);
					return;
				}
				it++;
			}
		}
		
	}

	void resetSim()
	{
		if (GetKey(olc::R).bPressed)
		{
			points.clear();
			sticks.clear();
			selectedPoint = nullptr;
			OnUserCreate();
		}
	}

	void connectPoints()
	{
		float x = GetMouseX();
		float y = GetMouseY();
		olc::vf2d mousePos = { x,y };

		if (GetMouse(1).bPressed)
		{ 
			for (auto& p : points)
			{
				float distance2 = (x - p->postion.x) * (x - p->postion.x) + (y - p->postion.y) * (y - p->postion.y);
				if (distance2 < 25.0f)
				{
					selectedPoint = p;
					break;
				}
			}
		}

		if (GetMouse(1).bHeld)
		{
			if (selectedPoint != nullptr)
			{
				DrawLine(mousePos, selectedPoint->postion);
			}
		}

		if (GetMouse(1).bReleased)
		{
			for (auto& p : points)
			{
				float distance2 = (x - p->postion.x) * (x - p->postion.x) + (y - p->postion.y) * (y - p->postion.y);
				if (distance2 < 25.0f && selectedPoint != nullptr  && selectedPoint!=p)
				{
					sticks.emplace_back(selectedPoint, p);
				}
			}
			selectedPoint = nullptr;
		}

	}

	void addPoint()
	{
		if (GetMouse(0).bPressed)
		{
			float x = GetMouseX();
			float y = GetMouseY();
			
			for (auto& p : points)
			{
				float distance2 = (x - p->postion.x) * (x - p->postion.x) + (y - p->postion.y) * (y - p->postion.y);
				if (distance2 < 25.0f)
				{
					p->locked = !p->locked;
					return;
				}
			}

			std::shared_ptr<Point> p = std::make_shared<Point>(Point{ olc::vf2d  { x,y }, olc::vf2d { x,y } });
			points.emplace_back(p);
		}
	}

	void updateSticks()
	{
		for (auto& s : sticks)
		{
			olc::vf2d distVector = s.PointA->postion - s.PointB->postion;
			float distance = distVector.mag();
			olc::vf2d normVector = distVector / distance;
			
			if(!s.PointA->locked)
				s.PointA->postion -= normVector * ((distance - s.lenght) / 2.0f);
			if (!s.PointB->locked)
				s.PointB->postion += normVector * ((distance - s.lenght) / 2.0f);
		}		
	}

	void updatePoints()
	{
		for (auto& p : points)
		{
			if (!p->locked)
			{
				olc::vf2d v = p->postion - p->prevPostion;
				p->prevPostion = p->postion;
				p->postion += v * friction;
				p->postion.y += gravity;
			}
		}
	}

	void constrainPoints()
	{
		for (auto& p : points)
		{
			if (!p->locked)
			{
				olc::vf2d v = p->postion - p->prevPostion;
				
				if (p->postion.x > ScreenWidth())
				{
					p->postion.x = ScreenWidth();
					p->prevPostion.x = p->postion.x + v.x * bounce;
				}
				else if (p->postion.x < 0)
				{
					p->postion.x = 0;
					p->prevPostion.x = p->postion.x + v.x * bounce;
				}
				if (p->postion.y > ScreenHeight())
				{
					p->postion.y = ScreenHeight();
					p->prevPostion.y = p->postion.y + v.y * bounce;
				}
				else if (p->postion.y < 0)
				{
					p->postion.y = 0;
					p->prevPostion.y = p->postion.y + v.y * bounce;
				}
			}
		}
	}



	ClothSim()
	{
		sAppName = "ClothSim";
	}

	bool OnUserCreate() override
	{
		
		constexpr int32_t pointColl = 25;
		constexpr int32_t pointRows = 15;

	
		for (int y = 0 ; y < pointRows ; y++)
		{
			for (int x = 0; x < pointColl; x++)
			{
				float pox = x * 30.0f + 37.0f;
				float poy = y * 30.0f + 30.0f;				

				std::shared_ptr<Point> p = std::make_shared<Point>(Point{ olc::vf2d  { pox, poy }, olc::vf2d { pox, poy } });
				points.emplace_back(p);
			}
		}

		for (int x = 0; x < pointColl; x++)
		{
			for (int y = 0; y < pointRows; y++)
			{
				int index = x + y * pointColl;
				
				//Simple lambda for calculating offeset
				auto offset = [&](int xo, int yo)
				{
					int in = (xo + x + (yo + y) * pointColl);
					return in;
				};
					
				
				if (x < pointColl - 1)
					sticks.emplace_back(points[index], points[offset(1, 0)]);
				if(y < pointRows - 1)
					sticks.emplace_back(points[index], points[offset(0, 1)]);			

			}
		}
		
		for (int i = 0; i < pointColl; i+=2)
		{
			points[i]->locked = true;
		}
		

		//Nice bug
		
		/*
		int pointColl = 14;
		int pointRows = 10;


		for (int x = 0 ; x < pointColl; x++)
		{
			for (int y = 0; y < pointRows; y++)
			{
				float pox = x * 30.0f + 30.0f;
				float poy = y * 30.0f + 30.0f;

				std::shared_ptr<Point> p = std::make_shared<Point>(Point{ olc::vf2d  { pox, poy }, olc::vf2d { pox, poy } });
				points.emplace_back(p);
			}
		}

		for (int x = 0; x < pointColl; x++)
		{
			for (int y = 0; y < pointRows; y++)
			{
				int index = x + y * pointColl;

				//Simple lambda for calculating offeset
				auto offset = [&](int xo, int yo)
				{
					int in = (xo + x + (yo + y) * pointColl);
					return in;
				};


				if (x < pointColl - 1)
					sticks.emplace_back(points[index], points[offset(1, 0)]);
				if (y < pointRows - 1)
					sticks.emplace_back(points[index], points[offset(0, 1)]);

			}
		}
		*/
		
		
		
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		//addPoint();
		//connectPoints();
		//removePoint();
		//movePoints();
		removeStick();
		resetSim();
		if (GetKey(olc::SPACE).bPressed)
		{
			doSim = !doSim;
		}


		AccumulatedTime += fElapsedTime;
		if (AccumulatedTime >= TargetFrameTime)
		{
			AccumulatedTime -= TargetFrameTime;
			fElapsedTime = TargetFrameTime;
			


			//UPDATE
			if (doSim)
			{
				updatePoints();
				for (int i = 0; i < 3; i++)
				{
					updateSticks();
					constrainPoints();
				}
			}


			
			//RENDER
			Clear(olc::Pixel (40, 114, 64));
			//Render points
			for (const auto& p : points)
			{
				if (p->locked)
					FillCircle(p->postion, 3, olc::Pixel(217, 66, 72));
				else
					FillCircle(p->postion, 3, olc::Pixel(216, 216, 216));
			}
			//Render stics
			for (const auto& s : sticks)
			{
				DrawLine(s.PointA->postion, s.PointB->postion, olc::Pixel(216, 216, 216));
			}

			return true;
		}
		else
		{
			return true;
		}


	}
};

int main()
{
	ClothSim clothSim;
	if (clothSim.Construct(800, 500, 2, 2))
		clothSim.Start();

	return 0;
}