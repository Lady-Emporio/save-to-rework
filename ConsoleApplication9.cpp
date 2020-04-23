// ConsoleApplication9.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//


#include <iostream>
#include <string>
#include <SFML/Network.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include "someFunc.h"
#include <windows.h>
#include <chrono>
#include "Managers.h"
#include <thread>
#include <mutex>

inline std::mutex manager_mutex;

/*
1) update now in come message
2) change ip ( name )
*/
int main()
{
	sf::RenderWindow  window(sf::VideoMode(800, 600), "My window");
	window.setFramerateLimit(10);

	sf::Font font;
	if (!font.loadFromFile("C:/Users/al/Desktop/1/18938.ttf"))
	{
		std::cout << "Can not load font." <<std::endl;
		return 2;
	}


	ManagerOnline * manager=ManagerOnline::getManager();
	run_daemon_server();
	run_daemon_timeout();
	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			switch (event.type) {
			case sf::Event::Closed:
				window.close();
				break;
			case sf::Event::KeyPressed:
				std::cout << "" << std::endl;
				break;
			}
		}

		window.clear(sf::Color::Black);

		int y = 10;
		{
			std::lock_guard lock(manager_mutex);

			for (int i = 0; i != manager->size();++i) {
				ManagerLiveSocketPtr t = (*manager)[i];
				sf::Text text(t->name, font);
				text.setCharacterSize(30);
				text.setStyle(sf::Text::Bold);
				text.setFillColor(sf::Color::Red);
				text.setPosition(10, y);
				window.draw(text);
				y += 20;
			}
		}
		window.display();
	}
	return 0;
}


/*
Нужно:
	имя
	сокет
	дата проверки


Необходимо:
	1 - автоупорядочивание по добавлению.
		каждые 360 итераций главного цикла обходим и ищем те, у которых дата истекла.
		Если дата истекла - грохаем.
	2 - обращение по имени. []
		Приходит сообщение от Пети к Васе. Нужно узнать сокет ["Васи"] и отправить ему сообщение.
*/
	