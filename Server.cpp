﻿#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <windows.h>
#include <ws2tcpip.h>
#include <conio.h>
#include <string>
#include <ctime>
#include <iomanip>
#include <thread>
#include <chrono>
#include <sstream>
using namespace std;

#pragma comment (lib, "Ws2_32.lib")
#pragma warning (disable:4996)

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015" 

#define PAUSE 1

SOCKET ClientSocket = INVALID_SOCKET;

COORD server_smile;
COORD client_smile;

int server_coin_count = 0;
int client_coin_count = 0;

bool timeUp = false; 
char** map = nullptr;


enum class KeyCodes { LEFT = 75, RIGHT = 77, UP = 72, DOWN = 80, ENTER = 13, ESCAPE = 27, SPACE = 32, ARROWS = 224 };
enum class Colors { BLUE = 9, RED = 12, BLACK = 0, YELLOW = 14, DARKGREEN = 2 };

bool collision = false;

void UpdateGameTimeInWindowTitle() 
{
	auto start = chrono::system_clock::now(); 

	while (!timeUp) { 
		auto now = chrono::system_clock::now();
		auto elapsed_minutes = chrono::duration_cast<chrono::minutes>(now - start); 
		auto elapsed_seconds = chrono::duration_cast<chrono::seconds>(now - start); 

		if (elapsed_minutes.count() >= 1) 
		{ 
			auto now = chrono::system_clock::now(); 
			auto elapsed_seconds = chrono::duration_cast<chrono::seconds>(now - start);
			timeUp = true;
			string coinCountMessage = "SERVER_COINS:" + to_string(server_coin_count);
			int iSendResult = send(ClientSocket, coinCountMessage.c_str(), coinCountMessage.size(), 0);
			if (iSendResult == SOCKET_ERROR) 
			{
				cout << "send failed with error: " << WSAGetLastError() << "\n";
				closesocket(ClientSocket);
				WSACleanup();
				return;
			}
			break; 
		}

		stringstream ss;
		ss << setw(2) << setfill('0') << elapsed_minutes.count() / 60 << ":"; 
		ss << setw(2) << setfill('0') << elapsed_minutes.count() % 60 << ":"; 
		ss << setw(2) << setfill('0') << elapsed_seconds.count() % 60; 
		string timeStr = ss.str();
		SetConsoleTitleA(timeStr.c_str());

		this_thread::sleep_for(chrono::seconds(1));
	}

	system("cls");

	cout << "SERVER: " << server_coin_count << endl;
	cout << "CLIENT: " << client_coin_count << endl;

}

void UpdateClientGameTimeInWindowTitle(const string& timeStr) 
{
	SetConsoleTitleA(timeStr.c_str()); 
}

void GenerateMap(char**& map, const unsigned int height, const unsigned int width) 
{
	map = new char* [height]; 
	for (int y = 0; y < height; y++) 
	{
		map[y] = new char[width]; 
		for (int x = 0; x < width; x++) 
		{
			map[y][x] = ' '; 


			int r = rand() % 10; 
			if (r == 0)
				map[y][x] = '.';

			if (x == 0 || x == width - 1 || y == 0 || y == height - 1)
				map[y][x] = '#'; 
			if (x == width - 1 && y == height / 2)
				map[y][x] = ' '; 
			if (x == 1 && y == 1) 
			{
				map[y][x] = '1'; 
				server_smile.X = x;
				server_smile.Y = y;
			}
			if (x == 1 && y == 3) 
			{
				map[y][x] = '2'; 
				client_smile.X = x;
				client_smile.Y = y;
			}
			r = rand() % 10; 
			if (r == 0 && map[y][x] != '1' && map[y][x] != '2')
				map[y][x] = '#';
		}
	}
}

void ShowMap(char** map, const unsigned int height, const unsigned int width) 
{
	setlocale(0, "C");
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
	for (int y = 0; y < height; y++) 
	{
		for (int x = 0; x < width; x++) 
		{
			if (map[y][x] == ' ')
			{
				SetConsoleTextAttribute(h, WORD(Colors::BLACK));
				cout << " ";
			}
			else if (map[y][x] == '#')
			{
				SetConsoleTextAttribute(h, WORD(Colors::DARKGREEN));
				cout << (char)219;
			}
			else if (map[y][x] == '1')
			{
				SetConsoleTextAttribute(h, WORD(Colors::RED));
				cout << (char)160;
			}
			else if (map[y][x] == '2')
			{
				SetConsoleTextAttribute(h, WORD(Colors::BLUE));
				cout << (char)160; 
			}
			else if (map[y][x] == '.')
			{
				SetConsoleTextAttribute(h, WORD(Colors::YELLOW));
				cout << "."; 
			}
		}
		cout << "\n"; 
	}
}

string MakeMessage(char** map, const unsigned int height, const unsigned int width)
{
	string message = "h";
	message += to_string(height);
	message += "w";
	message += to_string(width);
	message += "d"; 
	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
			message += map[y][x];
	return message;
}

void HandleCoinCollision(char** map, int x, int y)
{
	if (map[y][x] == '.') {
		if (x == server_smile.X && y == server_smile.Y)
		{
			server_coin_count++;
			map[y][x] = ' ';
		}
		else if (x == client_smile.X && y == client_smile.Y) 
		{
			client_coin_count++;
			map[y][x] = ' ';
		}
	}

}


DWORD WINAPI Sender(void* param)
{
	unsigned int height = 15; 
	unsigned int width = 30; 

	char** map = nullptr; 
	GenerateMap(map, height, width);
	system("cls");
	ShowMap(map, height, width);
	/////////////////////////
	COORD last_smile_position = server_smile;
	/////////////////////
	string message = MakeMessage(map, height, width);

	int iSendResult = send(ClientSocket, message.c_str(), message.size(), 0); 
	if (iSendResult == SOCKET_ERROR) {
		cout << "send failed with error: " << WSAGetLastError() << "\n";
		cout << "упс, отправка (send) ответного сообщения не состоялась ((\n";
		closesocket(ClientSocket);
		WSACleanup();
		return 7;
	}

	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_CURSOR_INFO cursor;
	cursor.bVisible = false;
	cursor.dwSize = 100;
	SetConsoleCursorInfo(h, &cursor);

	auto start = chrono::system_clock::now();

	while (true) 
	{

		KeyCodes code = (KeyCodes)_getch();
		if (code == KeyCodes::ARROWS)
			code = (KeyCodes)_getch(); 
		SetConsoleCursorPosition(h, server_smile);
		cout << " ";

		int direction = 0; 

		if (code == KeyCodes::LEFT && map[server_smile.Y][server_smile.X - 1] != '#') 
		{
			server_smile.X--;
			HandleCoinCollision(map, server_smile.X, server_smile.Y);
			direction = 1;
		}
		else if (code == KeyCodes::RIGHT && map[server_smile.Y][server_smile.X + 1] != '#')
		{
			server_smile.X++;
			HandleCoinCollision(map, server_smile.X, server_smile.Y);
			direction = 2;
		}
		else if (code == KeyCodes::UP && map[server_smile.Y - 1][server_smile.X] != '#') 
		{
			server_smile.Y--;
			HandleCoinCollision(map, server_smile.X, server_smile.Y);
			direction = 3;
		}
		else if (code == KeyCodes::DOWN && map[server_smile.Y + 1][server_smile.X] != '#') 
		{
			server_smile.Y++;
			HandleCoinCollision(map, server_smile.X, server_smile.Y);
			direction = 4;
		}

		SetConsoleCursorPosition(h, server_smile);
		SetConsoleTextAttribute(h, WORD(Colors::RED));
		cout << (char)160;

		HWND hwnd = GetConsoleWindow();

		if ((server_smile.X == client_smile.X && server_smile.Y == client_smile.Y) ||
			(abs(server_smile.X - client_smile.X) <= 1 && abs(server_smile.Y - client_smile.Y) <= 1)) {
			system("title Meeting!!");
		}
		else 
		{
			system("title SERVER SIDE");
		}



		char message[200];
		strcpy_s(message, 199, to_string(direction).c_str());
		strcat_s(message, 199, to_string(client_coin_count).c_str());
		int iResult = send(ClientSocket, message, (int)strlen(message), 0);

		if (iResult == SOCKET_ERROR)
		{
			cout << "send failed with error: " << WSAGetLastError() << "\n";
			closesocket(ClientSocket);
			WSACleanup();
			return 15;
		}
	}
	return 0;
}


void HandleServerCoinsMessage(const string& message) 
{
	size_t pos = message.find(':');
	if (pos != string::npos && pos + 1 < message.size()) 
	{
		string coinsStr = message.substr(pos + 1);
		int serverCoins = stoi(coinsStr);
		cout << "Server collected coins: " << serverCoins << endl;
	}
}


DWORD WINAPI Receiver(void* param)
{
	HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);



	while (true) {
		char answer[DEFAULT_BUFLEN];

		int iResult = recv(ClientSocket, answer, DEFAULT_BUFLEN, 0);
		answer[iResult] = '\0';

		string message = answer;

		if (message.substr(0, 12) == "SERVER_COINS") 
		{
			HandleServerCoinsMessage(message);
		}

		UpdateClientGameTimeInWindowTitle(message);

		SetConsoleCursorPosition(h, client_smile);
		SetConsoleTextAttribute(h, WORD(Colors::BLACK));
		cout << " "; \

		COORD temp_server_smile = server_smile;

		if (message == "1")
		{ 
			client_smile.X--;


		}
		else if (message == "2") 
		{ 
			client_smile.X++;

		}
		else if (message == "3") 
		{ 
			client_smile.Y--;

		}
		else if (message == "4") 
		{ 
			client_smile.Y++;

		}

		SetConsoleCursorPosition(h, client_smile);
		SetConsoleTextAttribute(h, WORD(Colors::BLUE));
		cout << (char)160; // cout << "@";

		HWND hwnd = GetConsoleWindow();

		if ((server_smile.X == client_smile.X && server_smile.Y == client_smile.Y) ||
			(abs(server_smile.X - client_smile.X) <= 1 && abs(server_smile.Y - client_smile.Y) <= 1)) {
			system("title Meeting!!");
		}
		else 
		{
			system("title SERVER SIDE");
		}

		if (iResult > 0) 
		{
		}
		else if (iResult == 0)
			cout << "соединение с сервером закрыто.\n";
		else
			cout << "recv failed with error: " << WSAGetLastError() << "\n";
	}
	return 0;
}

void UpdateWindowTitle(HWND hwnd, const char* title) 
{
	SetWindowTextA(hwnd, title); 
}

int main()
{
	setlocale(0, "");
	system("title SERVER SIDE");
	system("color 07");
	system("cls");
	MoveWindow(GetConsoleWindow(), 650, 50, 500, 500, true);
	thread timeThread(UpdateGameTimeInWindowTitle);

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	
	WSADATA wsaData; 
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		cout << "WSAStartup failed with error: " << iResult << "\n";
		cout << "подключение Winsock.dll прошло с ошибкой!\n";
		return 1;
	}
	else 
	{
		Sleep(PAUSE);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	struct addrinfo hints; 
	hints.ai_family = AF_INET; 
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP; 

	struct addrinfo* result = NULL;
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0)
	{
		cout << "getaddrinfo failed with error: " << iResult << "\n";
		cout << "получение адреса и порта сервера прошло c ошибкой!\n";
		WSACleanup(); 
		return 2;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	SOCKET ListenSocket = INVALID_SOCKET;
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) 
	{
		cout << "socket failed with error: " << WSAGetLastError() << "\n";
		cout << "создание сокета прошло c ошибкой!\n";
		freeaddrinfo(result);
		WSACleanup();

		return 3;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	{
		cout << "bind failed with error: " << WSAGetLastError() << "\n";
		cout << "внедрение сокета по IP-адресу прошло с ошибкой!\n";
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 4;
	}

	freeaddrinfo(result);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) 
	{
		cout << "listen failed with error: " << WSAGetLastError() << "\n";
		cout << "прослушка информации от клиента не началась. что-то пошло не так!\n";
		closesocket(ListenSocket);
		WSACleanup();
		return 5;
	}
	else 
	{
		cout << "пожалуйста, запустите client.exe\n";
		system("start D:\\ITSTEP\\OOP\\GameSmile\\x64\\Debug\\Client.exe");
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	ClientSocket = accept(ListenSocket, NULL, NULL); 
	if (ClientSocket == INVALID_SOCKET)
	{
		cout << "accept failed with error: " << WSAGetLastError() << "\n";
		cout << "соединение с клиентским приложением не установлено! печаль!\n";
		closesocket(ListenSocket);
		WSACleanup();
		return 6;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////


	CreateThread(0, 0, Receiver, 0, 0, 0);
	CreateThread(0, 0, Sender, 0, 0, 0);
	timeThread.join();

	Sleep(INFINITE);
}