#include <SFML/Graphics.hpp>
#include <iostream>
#include "imgui-SFML.h"
#include "imgui.h"

#include <array>
#include <functional>
#include <vector>

sf::Color interpolateColors(const sf::Color &color1, const sf::Color &color2, float t)
{
	float r = color1.r + (color2.r - color1.r) * t;
	float g = color1.g + (color2.g - color1.g) * t;
	float b = color1.b + (color2.b - color1.b) * t;
	float a = color1.a + (color2.a - color1.a) * t;

	return sf::Color(static_cast<sf::Uint8>(r), static_cast<sf::Uint8>(g), static_cast<sf::Uint8>(b),
					 static_cast<sf::Uint8>(a));
}

float createMatrix(const std::vector<std::vector<float>> &matrix)
{
	return matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[1][2] * matrix[2][1]) -
		matrix[0][1] * (matrix[1][0] * matrix[2][2] - matrix[1][2] * matrix[2][0]) +
		matrix[0][2] * (matrix[1][0] * matrix[2][1] - matrix[1][1] * matrix[2][0]);
}

class RClass : public sf::Sprite
{
public:
	void	Create(const sf::Vector2u &size, const int selectedNormalIndex)
	{
		image.create(size.x, size.y);
		texture.loadFromImage(image);
		setTexture(texture);

		fColor = sf::Color::Black;
		sColor = sf::Color::White;
		_cx.resize(600, std::vector<float>(600));
		_cy.resize(600, std::vector<float>(600));

		index = selectedNormalIndex;
	}

	void DrawRFunc(const std::function<float(const sf::Vector2f &)> &rfunc, const sf::FloatRect &subSpace)
	{
		sf::Vector2f spaceStep = {subSpace.width / static_cast<float>(image.getSize().x),
								  subSpace.height / static_cast<float>(image.getSize().y)};

		for (int x = 0; x < image.getSize().x; ++x)
		{
			for (int y = 0; y < image.getSize().y; ++y)
			{
				sf::Vector2f spacePointFirst = {subSpace.left + static_cast<float>(x) * spaceStep.x,
												subSpace.top + static_cast<float>(y) * spaceStep.y};

				const float z1 = rfunc(spacePointFirst);

				sf::Vector2f spacePointSecond = {subSpace.left + static_cast<float>(x + 1) * spaceStep.x,
												 subSpace.top + static_cast<float>(y) * spaceStep.y};

				const float z2 = rfunc(spacePointSecond);

				sf::Vector2f spacePointThird = {subSpace.left + static_cast<float>(x) * spaceStep.x,
												subSpace.top + static_cast<float>(y + 1) * spaceStep.y};

				const float z3 = rfunc(spacePointThird);

				const float A = createMatrix({
					{spacePointFirst.y, z1, 1},
					{spacePointSecond.y, z2, 1},
					{spacePointThird.y, z3, 1},
				});

				const float B = createMatrix({
					{spacePointFirst.x, z1, 1},
					{spacePointSecond.x, z2, 1},
					{spacePointThird.x, z3, 1},
				});

				const float C = createMatrix({
					{spacePointFirst.x, spacePointFirst.y, 1},
					{spacePointSecond.x, spacePointSecond.y, 1},
					{spacePointThird.x, spacePointThird.y, 1},
				});

				const float D = createMatrix({
					{spacePointFirst.x, spacePointFirst.y, z1},
					{spacePointSecond.x, spacePointSecond.y, z2},
					{spacePointThird.x, spacePointThird.y, z3},
				});

				const float rat = std::sqrt(A * A + B * B + C * C + D * D);

				float nx = A / rat, ny = B / rat, nz = C / rat, nw = D / rat;
				_cx[x][y] = nx;
				_cy[x][y] = ny;
				float selectedNormal = nx;

				switch (index)
				{
				case 0:
					break;
				case 1:
					selectedNormal = ny;
					break;
				case 2:
					selectedNormal = nz;
					break;
				case 3:
					selectedNormal = nw;
					break;
				}

				auto pixelColor = interpolateColors(fColor, sColor, (1.f + selectedNormal) / 2);
				image.setPixel(x, y, pixelColor);
			}
		}

		texture.update(image);
	}

	void UpdatePalette(const sf::Color &firstColor, const sf::Color &secondColor)
	{
		for (int x = 0; x < image.getSize().x - 1; ++x)
		{
			for (int y = 0; y < image.getSize().y - 1; ++y)
			{
				float t = (static_cast<float>(image.getPixel(x, y).r) - fColor.r) / (sColor.r - fColor.r);
				auto pixelColor = interpolateColors(firstColor, secondColor, t);
				image.setPixel(x, y, pixelColor);
			}
		}

		fColor = firstColor;
		sColor = secondColor;
		texture.update(image);
	}

	void SaveImageToFile(const std::string &filename) { image.saveToFile(filename); }

	std::vector<std::vector<float>> _cx;
	std::vector<std::vector<float>> _cy;

private:
	sf::Color fColor;
	sf::Color sColor;
	sf::Texture texture;
	sf::Image image;
	int index;
};

class gSprite : public sf::Sprite
{
public:
	void Create(const sf::Vector2u &size)
	{
		_texture.create(size.x, size.y);
		setTexture(_texture.getTexture());
	}
	void Clear() { _texture.clear(sf::Color::Transparent); }
	void Draw(sf::Vector2u point, sf::Color gradientColor, RClass &r, int direction)
	{
		std::vector<std::vector<float>> UM(600, std::vector<float>(600));
		while (point.x > 0 && point.y > 0 && point.x < _texture.getSize().x && point.y < _texture.getSize().y &&
			   !UM[point.x][point.y])
		{
			float cx = r._cx[point.x][point.y];
			float cy = r._cy[point.x][point.y];

			sf::RectangleShape currentStep({2, 2});
			currentStep.setFillColor(gradientColor);
			currentStep.setPosition(point.x, _texture.getSize().y - point.y);
			_texture.draw(currentStep);
			UM[point.x][point.y] = true;

			point.x += static_cast<unsigned>(10 * cx * direction);
			point.y += static_cast<unsigned>(10 * cy * direction);
		}
	}

private:
	sf::RenderTexture _texture;
};

float RAnd(float w1, float w2) { return w1 + w2 + std::sqrt((w1 * w1 + w2 * w2) - 2 * w1 * w2); }

float ROr(float w1, float w2) { return w1 + w2 - std::sqrt((w1 * w1 + w2 * w2) - 2 * w1 * w2); }

int currentsprite = 0;
std::vector<RClass *> sprites;

int main()
{

	sf::RenderWindow window(sf::VideoMode(600, 600), "cpp-lab-3");
	window.setFramerateLimit(60);
	if (!ImGui::SFML::Init(window))
	{
		std::cout << "ImGui initialization failed";
		return -1;
	}

	auto spriteSize = sf::Vector2u{window.getSize().x, window.getSize().y};

	RClass rClassNX;
	rClassNX.Create(spriteSize, 0);
	sprites.push_back(&rClassNX);

	RClass rClassNY;
	rClassNY.Create(spriteSize, 1);
	sprites.push_back(&rClassNY);

	RClass rClassNZ;
	rClassNZ.Create(spriteSize, 2);
	sprites.push_back(&rClassNZ);

	RClass rClassNW;
	rClassNW.Create(spriteSize, 3);
	sprites.push_back(&rClassNW);

	gSprite gsprite;
	gsprite.Create(sf::Vector2u(600, 600));

	std::function<float(const sf::Vector2f &)> rFunction[5];

	rFunction[0] = [](const sf::Vector2f &point) -> float { return std::sin(point.x) + std::cos(point.y); };
	rFunction[1] = [](const sf::Vector2f &point) -> float { return std::cos(point.x) * std::sin(point.y); };
	rFunction[2] = [](const sf::Vector2f &point) -> float { return std::cos(point.x + point.y); };
	rFunction[3] = [](const sf::Vector2f &point) -> float { return point.x * point.x + point.y * point.y - 200; };
	rFunction[4] = [](const sf::Vector2f &point) -> float { return std::sin(point.x) * std::cos(point.y); };

	std::function<float(const sf::Vector2f &)> complexFunction = [&rFunction](const sf::Vector2f &point) -> float
	{
		return RAnd(RAnd(ROr(RAnd(rFunction[0](point), rFunction[1](point)), rFunction[2](point)), rFunction[3](point)),
					ROr(rFunction[4](point), rFunction[0](point)));
	};

	sf::FloatRect subSpace(-10.f, -10.f, 20.f, 20.f);

	static ImVec4 firstColor(0, 0, 0, 1);
	static ImVec4 secondColor(1, 1, 1, 1);

	for (int i = 0; i < sprites.size(); i++)
	{
		sprites[i]->DrawRFunc(complexFunction, subSpace);
	}

	sf::Clock deltaClock;

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			ImGui::SFML::ProcessEvent(event);

			if (event.type == sf::Event::Closed)
			{
				window.close();
			}
			if (event.type == sf::Event::MouseButtonPressed)
			{
				if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
				{
					gsprite.Draw(sf::Vector2u(static_cast<unsigned>(event.mouseButton.x),
											  static_cast<unsigned>(event.mouseButton.y)),
								 sf::Color::Yellow, *sprites[0], 1);
				}
				else if (sf::Mouse::isButtonPressed(sf::Mouse::Right))
				{
					gsprite.Draw(sf::Vector2u(static_cast<unsigned>(event.mouseButton.x),
											  static_cast<unsigned>(event.mouseButton.y)),
								 sf::Color::Magenta, *sprites[0], -1);
				}
			}
		}

		ImGui::SFML::Update(window, deltaClock.restart());

		static bool mchanged = false;
		ImGui::Begin("Controls");
		if (ImGui::Button("OX"))
		{
			currentsprite = 0;
			mchanged = true;
		}

		ImGui::SameLine();
		if (ImGui::Button("OY"))
		{
			currentsprite = 1;
			mchanged = true;
		}

		if (ImGui::Button("OZ"))
		{
			currentsprite = 2;
			mchanged = true;
		}

		ImGui::SameLine();
		if (ImGui::Button("OW"))
		{
			currentsprite = 3;
			mchanged = true;
		}

		if (ImGui::ColorEdit3("First color", &firstColor.x))
		{
		}
		if (ImGui::ColorEdit3("Second color", &secondColor.x))
		{
		}

		if (ImGui::Button("Update"))
		{
			auto sfFirstColor =
				sf::Color(static_cast<sf::Uint8>(firstColor.x * 255), static_cast<sf::Uint8>(firstColor.y * 255),
						  static_cast<sf::Uint8>(firstColor.z * 255), static_cast<sf::Uint8>(firstColor.w * 255));

			auto sfSecondColor =
				sf::Color(static_cast<sf::Uint8>(secondColor.x * 255), static_cast<sf::Uint8>(secondColor.y * 255),
						  static_cast<sf::Uint8>(secondColor.z * 255), static_cast<sf::Uint8>(secondColor.w * 255));

			for (RClass *sprite : sprites)
			{
				sprite->UpdatePalette(sfFirstColor, sfSecondColor);
			}
		}
		if (ImGui::Button("Clear gradient"))
		{
			gsprite.Clear();
		}


		ImGui::Text("Press 'S' to save nx,ny,nz,nw images");

		if (event.key.code == sf::Keyboard::S)
		{
			rClassNX.SaveImageToFile("images/nx.png");
			rClassNY.SaveImageToFile("images/ny.png");
			rClassNZ.SaveImageToFile("images/nz.png");
			rClassNW.SaveImageToFile("images/nw.png");
		}

		ImGui::End();

		window.clear();
		if (mchanged)
		{
			gsprite.Clear();
			mchanged = false;
		}
		if (currentsprite == 0)
		{
			window.draw(rClassNX);
		}
		else if (currentsprite == 1)
		{
			window.draw(rClassNY);
		}
		else if (currentsprite == 2)
		{
			window.draw(rClassNZ);
		}
		else if (currentsprite == 3)
		{
			window.draw(rClassNW);
		}
		window.draw(gsprite);

		ImGui::SFML::Render(window);

		window.display();
	}

	ImGui::SFML::Shutdown();

	return 0;
}