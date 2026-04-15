// clang++ main.cpp -o desktopSnake.exe -lUser32 -std=gnu++23 -Wl,/SUBSYSTEM:WINDOWS

#include <Windows.h>
#include <CommCtrl.h>
#include <vector>
#include <ctime>
#include <format>

#define moveIcon(i, x, y) ListView_SetItemPosition(listView, i, x, y)

struct Point {
	int x, y;
};

enum Direction { UP, DOWN, LEFT, RIGHT };

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	// initial stuff
	HWND listView = FindWindowEx(FindWindowEx(FindWindowEx(GetDesktopWindow(), NULL, "Progman", NULL), NULL, "SHELLDLL_DefView", NULL), NULL, "SysListView32", NULL);
	
	DWORD dwordcellSize = ListView_GetItemSpacing(listView, FALSE);
	Point cellSize = {LOWORD(dwordcellSize), HIWORD(dwordcellSize)};

	Point gridSize = {27, 11};

	// Save icon positions
	// stupid memory trick because the stupid getter function takes a stupid pointer instead of just returning the stupid value!!
	DWORD pid;
	GetWindowThreadProcessId(listView, &pid);
	HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, pid);
	int iconCount = ListView_GetItemCount(listView);
	POINT *remotePoints = (POINT*)VirtualAllocEx(hProcess, nullptr, sizeof(POINT) * iconCount, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	for (int i = 0; i < iconCount; ++i) { ListView_GetItemPosition(listView, i, remotePoints + i); }
	std::vector<POINT> originalPositions(iconCount);
	ReadProcessMemory(hProcess, remotePoints, originalPositions.data(), sizeof(POINT) * iconCount, nullptr);
	VirtualFreeEx(hProcess, remotePoints, 0, MEM_RELEASE);
	CloseHandle(hProcess);
	
	// game
	srand(time(nullptr));

	std::vector<Point> snake = { {gridSize.x / 2, gridSize.y / 2} };
	Direction dir = RIGHT;

	Point food = { rand() % gridSize.x, rand() % gridSize.y };

	bool running = true;

	DWORD lastTick = GetTickCount();

	while (running) {
		// input
		if (GetAsyncKeyState(VK_UP) & 0x8000 && dir != DOWN) dir = UP;
		if (GetAsyncKeyState(VK_DOWN) & 0x8000 && dir != UP) dir = DOWN;
		if (GetAsyncKeyState(VK_LEFT) & 0x8000 && dir != RIGHT) dir = LEFT;
		if (GetAsyncKeyState(VK_RIGHT) & 0x8000 && dir != LEFT) dir = RIGHT;

		DWORD now = GetTickCount();
		if (now - lastTick > 100) {
			lastTick = now;

			// move
			Point head = snake.front();
			if (dir == UP) head.y--;
			if (dir == DOWN) head.y++;
			if (dir == LEFT) head.x--;
			if (dir == RIGHT) head.x++;

			// wall collision
			if (head.x < 0 || head.x >= gridSize.x || head.y < 0 || head.y >= gridSize.y) {
				running = false;
			}

			// self collision
			for (Point &p : snake) {
				if (p.x == head.x && p.y == head.y) {
					running = false;
				}
			}

			// new head
			snake.insert(snake.begin(), head);

			// food
			if (head.x == food.x && head.y == food.y) {
				// spawn new food
				food.x = rand() % gridSize.x;
				food.y = rand() % gridSize.y;
			} else {
				snake.pop_back(); // remove tail
			}

			// Render

			for (int i = 0; i < snake.size(); ++i) {
				moveIcon(i, snake[i].x * cellSize.x, snake[i].y * cellSize.y);
			}
			moveIcon(snake.size(), food.x * cellSize.x, food.y * cellSize.y);
		}

		Sleep(1);
	}
	
	MessageBox(nullptr, std::format("Game Over! Score: {}", snake.size() - 1).c_str(), "Desktop Snake", MB_OK | MB_ICONEXCLAMATION);

	// Restore icon positions
	for (int i = 0; i < originalPositions.size(); ++i) {
		ListView_SetItemPosition(listView, i, originalPositions[i].x, originalPositions[i].y);
	}
}