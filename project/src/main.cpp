#include <windows.h>

#include <algorithm>
#include <array>
#include <string>
#include <vector>

namespace {

constexpr int kBoardLeft = 54;
constexpr int kBoardTop = 164;
constexpr int kCellSize = 64;
constexpr int kBoardSize = kCellSize * 9;

const int kPuzzles[3][9][9] = {
    {
        {5, 3, 0, 0, 7, 0, 0, 0, 0},
        {6, 0, 0, 1, 9, 5, 0, 0, 0},
        {0, 9, 8, 0, 0, 0, 0, 6, 0},
        {8, 0, 0, 0, 6, 0, 0, 0, 3},
        {4, 0, 0, 8, 0, 3, 0, 0, 1},
        {7, 0, 0, 0, 2, 0, 0, 0, 6},
        {0, 6, 0, 0, 0, 0, 2, 8, 0},
        {0, 0, 0, 4, 1, 9, 0, 0, 5},
        {0, 0, 0, 0, 8, 0, 0, 7, 9}
    },
    {
        {0, 0, 0, 2, 6, 0, 7, 0, 1},
        {6, 8, 0, 0, 7, 0, 0, 9, 0},
        {1, 9, 0, 0, 0, 4, 5, 0, 0},
        {8, 2, 0, 1, 0, 0, 0, 4, 0},
        {0, 0, 4, 6, 0, 2, 9, 0, 0},
        {0, 5, 0, 0, 0, 3, 0, 2, 8},
        {0, 0, 9, 3, 0, 0, 0, 7, 4},
        {0, 4, 0, 0, 5, 0, 0, 3, 6},
        {7, 0, 3, 0, 1, 8, 0, 0, 0}
    },
    {
        {0, 2, 0, 6, 0, 8, 0, 0, 0},
        {5, 8, 0, 0, 0, 9, 7, 0, 0},
        {0, 0, 0, 0, 4, 0, 0, 0, 0},
        {3, 7, 0, 0, 0, 0, 5, 0, 0},
        {6, 0, 0, 0, 0, 0, 0, 0, 4},
        {0, 0, 8, 0, 0, 0, 0, 1, 3},
        {0, 0, 0, 0, 2, 0, 0, 0, 0},
        {0, 0, 9, 8, 0, 0, 0, 3, 6},
        {0, 0, 0, 3, 0, 6, 0, 9, 0}
    }
};

struct App {
    int board[9][9]{};
    int clues[9][9]{};
    bool conflicts[9][9]{};
    int selectedRow = 0;
    int selectedCol = 0;
    int puzzleIndex = 0;
    std::wstring status = L"Ready when you are.";
};

RECT MakeRect(int left, int top, int right, int bottom) {
    return RECT{left, top, right, bottom};
}

bool Contains(const RECT& rect, int x, int y) {
    return x >= rect.left && x < rect.right && y >= rect.top && y < rect.bottom;
}

void Fill(HDC dc, const RECT& rect, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(dc, &rect, brush);
    DeleteObject(brush);
}

void Rounded(HDC dc, const RECT& rect, COLORREF fill, COLORREF border, int radius = 18) {
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

HFONT Font(int height, int weight = FW_NORMAL) {
    return CreateFontW(height, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
}

void Label(HDC dc, const std::wstring& value, const RECT& rect, HFONT font, COLORREF color, UINT format) {
    HGDIOBJ oldFont = SelectObject(dc, font);
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);
    DrawTextW(dc, value.c_str(), -1, const_cast<RECT*>(&rect), format);
    SelectObject(dc, oldFont);
}

bool IsSafe(const int board[9][9], int row, int col, int value) {
    for (int index = 0; index < 9; ++index) {
        if (index != col && board[row][index] == value) {
            return false;
        }
        if (index != row && board[index][col] == value) {
            return false;
        }
    }
    const int startRow = row - row % 3;
    const int startCol = col - col % 3;
    for (int r = startRow; r < startRow + 3; ++r) {
        for (int c = startCol; c < startCol + 3; ++c) {
            if ((r != row || c != col) && board[r][c] == value) {
                return false;
            }
        }
    }
    return true;
}

bool IsValidBoard(const int board[9][9]) {
    for (int row = 0; row < 9; ++row) {
        for (int col = 0; col < 9; ++col) {
            const int value = board[row][col];
            if (value < 0 || value > 9 || (value != 0 && !IsSafe(board, row, col, value))) {
                return false;
            }
        }
    }
    return true;
}

std::vector<int> Candidates(const int board[9][9], int row, int col) {
    std::vector<int> values;
    for (int value = 1; value <= 9; ++value) {
        if (IsSafe(board, row, col, value)) {
            values.push_back(value);
        }
    }
    return values;
}

bool Solve(int board[9][9]) {
    int bestRow = -1;
    int bestCol = -1;
    std::vector<int> bestOptions;
    for (int row = 0; row < 9; ++row) {
        for (int col = 0; col < 9; ++col) {
            if (board[row][col] != 0) {
                continue;
            }
            auto options = Candidates(board, row, col);
            if (options.empty()) {
                return false;
            }
            if (bestRow == -1 || options.size() < bestOptions.size()) {
                bestRow = row;
                bestCol = col;
                bestOptions = std::move(options);
            }
        }
    }
    if (bestRow == -1) {
        return true;
    }
    for (int value : bestOptions) {
        board[bestRow][bestCol] = value;
        if (Solve(board)) {
            return true;
        }
        board[bestRow][bestCol] = 0;
    }
    return false;
}

void UpdateConflicts(App& app) {
    for (auto& row : app.conflicts) {
        std::fill(std::begin(row), std::end(row), false);
    }
    for (int row = 0; row < 9; ++row) {
        for (int col = 0; col < 9; ++col) {
            const int value = app.board[row][col];
            if (value != 0 && !IsSafe(app.board, row, col, value)) {
                app.conflicts[row][col] = true;
            }
        }
    }
}

bool Complete(const App& app) {
    for (const auto& row : app.board) {
        for (int value : row) {
            if (value == 0) {
                return false;
            }
        }
    }
    return true;
}

void LoadPuzzle(App& app, int index) {
    app.puzzleIndex = index % 3;
    for (int row = 0; row < 9; ++row) {
        for (int col = 0; col < 9; ++col) {
            app.board[row][col] = kPuzzles[app.puzzleIndex][row][col];
            app.clues[row][col] = kPuzzles[app.puzzleIndex][row][col];
        }
    }
    app.selectedRow = 0;
    app.selectedCol = 0;
    app.status = L"Fresh board loaded. Find its rhythm.";
    UpdateConflicts(app);
}

void ClearEntries(App& app) {
    for (int row = 0; row < 9; ++row) {
        for (int col = 0; col < 9; ++col) {
            if (app.clues[row][col] == 0) {
                app.board[row][col] = 0;
            }
        }
    }
    app.status = L"Editable cells cleared.";
    UpdateConflicts(app);
}

void EnterValue(App& app, int value) {
    if (app.clues[app.selectedRow][app.selectedCol] != 0) {
        app.status = L"That number is part of the original board.";
        return;
    }
    app.board[app.selectedRow][app.selectedCol] = value;
    UpdateConflicts(app);
    if (app.conflicts[app.selectedRow][app.selectedCol]) {
        app.status = L"That value clashes with the board.";
    } else if (Complete(app) && IsValidBoard(app.board)) {
        app.status = L"Beautiful. You solved it.";
    } else {
        app.status = value == 0 ? L"Cell cleared." : L"Value placed.";
    }
}

void DrawBackground(HDC dc, const RECT& canvas) {
    const int height = std::max(1L, canvas.bottom - canvas.top);
    for (int y = 0; y < height; y += 4) {
        const int shade = 16 + (y * 12) / height;
        Fill(dc, MakeRect(0, y, canvas.right, std::min(y + 4, height)), RGB(10, shade, 31 + shade / 2));
    }
    HBRUSH glow = CreateSolidBrush(RGB(20, 48, 72));
    HGDIOBJ old = SelectObject(dc, glow);
    Ellipse(dc, 780, -250, 1320, 300);
    SelectObject(dc, old);
    DeleteObject(glow);
    HBRUSH orb = CreateSolidBrush(RGB(32, 37, 78));
    old = SelectObject(dc, orb);
    Ellipse(dc, -230, 595, 260, 1075);
    SelectObject(dc, old);
    DeleteObject(orb);
}

void DrawButton(HDC dc, const RECT& rect, const std::wstring& text, bool primary) {
    const COLORREF fill = primary ? RGB(34, 211, 174) : RGB(27, 46, 68);
    const COLORREF border = primary ? RGB(108, 255, 218) : RGB(54, 80, 105);
    const COLORREF ink = primary ? RGB(6, 27, 34) : RGB(221, 235, 245);
    Rounded(dc, rect, fill, border, 16);
    HFONT font = Font(18, FW_SEMIBOLD);
    Label(dc, text, rect, font, ink, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    DeleteObject(font);
}

void DrawBoard(HDC dc, const App& app) {
    const RECT panel = MakeRect(28, 112, 656, 776);
    Rounded(dc, panel, RGB(16, 32, 50), RGB(57, 81, 105), 26);
    HFONT boardLabelFont = Font(14, FW_SEMIBOLD);
    Label(dc, L"THE BOARD", MakeRect(54, 130, 260, 155), boardLabelFont, RGB(132, 161, 183), DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DeleteObject(boardLabelFont);

    HBRUSH boardBrush = CreateSolidBrush(RGB(9, 22, 37));
    HGDIOBJ oldBrush = SelectObject(dc, boardBrush);
    HPEN outline = CreatePen(PS_SOLID, 2, RGB(89, 124, 146));
    HGDIOBJ oldPen = SelectObject(dc, outline);
    RoundRect(dc, kBoardLeft - 2, kBoardTop - 2, kBoardLeft + kBoardSize + 2, kBoardTop + kBoardSize + 2, 12, 12);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(boardBrush);
    DeleteObject(outline);

    const int selectedValue = app.board[app.selectedRow][app.selectedCol];
    for (int row = 0; row < 9; ++row) {
        for (int col = 0; col < 9; ++col) {
            RECT cell = MakeRect(kBoardLeft + col * kCellSize, kBoardTop + row * kCellSize,
                kBoardLeft + (col + 1) * kCellSize, kBoardTop + (row + 1) * kCellSize);
            COLORREF fill = ((row / 3 + col / 3) % 2 == 0) ? RGB(13, 31, 48) : RGB(15, 36, 54);
            const bool sharedUnit = row == app.selectedRow || col == app.selectedCol ||
                (row / 3 == app.selectedRow / 3 && col / 3 == app.selectedCol / 3);
            if (sharedUnit) {
                fill = RGB(19, 48, 66);
            }
            if (selectedValue != 0 && app.board[row][col] == selectedValue) {
                fill = RGB(26, 72, 82);
            }
            if (app.conflicts[row][col]) {
                fill = RGB(100, 43, 57);
            }
            if (row == app.selectedRow && col == app.selectedCol) {
                fill = RGB(67, 89, 141);
            }
            Fill(dc, cell, fill);
            if (app.board[row][col] != 0) {
                HFONT numberFont = Font(29, app.clues[row][col] ? FW_BOLD : FW_SEMIBOLD);
                const COLORREF ink = app.conflicts[row][col] ? RGB(255, 225, 226) :
                    (app.clues[row][col] ? RGB(244, 248, 251) : RGB(110, 234, 205));
                Label(dc, std::wstring(1, static_cast<wchar_t>(L'0' + app.board[row][col])), cell, numberFont, ink, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                DeleteObject(numberFont);
            }
        }
    }

    HPEN thin = CreatePen(PS_SOLID, 1, RGB(49, 78, 98));
    HPEN thick = CreatePen(PS_SOLID, 3, RGB(108, 144, 161));
    HGDIOBJ old = SelectObject(dc, thin);
    for (int line = 1; line < 9; ++line) {
        SelectObject(dc, line % 3 == 0 ? thick : thin);
        const int x = kBoardLeft + line * kCellSize;
        MoveToEx(dc, x, kBoardTop, nullptr);
        LineTo(dc, x, kBoardTop + kBoardSize);
        const int y = kBoardTop + line * kCellSize;
        MoveToEx(dc, kBoardLeft, y, nullptr);
        LineTo(dc, kBoardLeft + kBoardSize, y);
    }
    SelectObject(dc, old);
    DeleteObject(thin);
    DeleteObject(thick);
}

void DrawInterface(HDC dc, const RECT& canvas, const App& app) {
    DrawBackground(dc, canvas);

    HFONT eyebrow = Font(15, FW_BOLD);
    Label(dc, L"C++ / BACKTRACKING EDITION", MakeRect(54, 31, 420, 55), eyebrow, RGB(56, 227, 194), DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DeleteObject(eyebrow);
    HFONT title = Font(42, FW_BOLD);
    Label(dc, L"Sudoku Studio", MakeRect(52, 52, 560, 105), title, RGB(242, 247, 250), DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DeleteObject(title);
    HFONT subtitle = Font(17);
    Label(dc, L"A quiet space for loud logic.", MakeRect(650, 63, 1100, 91), subtitle, RGB(162, 186, 205), DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    DeleteObject(subtitle);

    DrawBoard(dc, app);

    Rounded(dc, MakeRect(690, 112, 1133, 776), RGB(17, 34, 53), RGB(57, 81, 105), 26);
    HFONT section = Font(15, FW_BOLD);
    Label(dc, L"CONTROL DESK", MakeRect(722, 142, 1050, 166), section, RGB(132, 161, 183), DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DeleteObject(section);
    HFONT actionTitle = Font(31, FW_BOLD);
    Label(dc, L"Solve with clarity.", MakeRect(720, 175, 1080, 218), actionTitle, RGB(240, 246, 250), DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DeleteObject(actionTitle);
    HFONT copy = Font(16);
    Label(dc, L"Choose a square, type a number,\nand let the engine handle the rest.", MakeRect(722, 220, 1085, 272), copy, RGB(159, 184, 203), DT_LEFT | DT_WORDBREAK);
    DeleteObject(copy);

    DrawButton(dc, MakeRect(722, 304, 1101, 360), L"Solve board", true);
    DrawButton(dc, MakeRect(722, 374, 903, 426), L"Reset", false);
    DrawButton(dc, MakeRect(920, 374, 1101, 426), L"New board", false);

    Rounded(dc, MakeRect(722, 462, 1101, 562), RGB(11, 25, 40), RGB(48, 75, 96), 16);
    HFONT statusLabel = Font(13, FW_BOLD);
    Label(dc, L"STATUS", MakeRect(742, 480, 900, 500), statusLabel, RGB(74, 222, 190), DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DeleteObject(statusLabel);
    HFONT status = Font(17, FW_SEMIBOLD);
    Label(dc, app.status, MakeRect(742, 506, 1078, 545), status, RGB(226, 238, 245), DT_LEFT | DT_WORDBREAK);
    DeleteObject(status);

    HFONT guideLabel = Font(13, FW_BOLD);
    Label(dc, L"QUICK KEYS", MakeRect(724, 594, 1000, 616), guideLabel, RGB(132, 161, 183), DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DeleteObject(guideLabel);
    HFONT guide = Font(16);
    Label(dc, L"1-9  enter a value\nDel  clear cell\nEnter  solve\nN  new board", MakeRect(724, 622, 1000, 720), guide, RGB(204, 219, 230), DT_LEFT | DT_WORDBREAK);
    DeleteObject(guide);

    HFONT footer = Font(13, FW_SEMIBOLD);
    Label(dc, L"ROW + COLUMN + BOX  /  MRV SEARCH", MakeRect(54, 788, 1080, 812), footer, RGB(101, 132, 153), DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DeleteObject(footer);
}

void Paint(HWND window, App& app) {
    PAINTSTRUCT paint{};
    HDC dc = BeginPaint(window, &paint);
    RECT canvas{};
    GetClientRect(window, &canvas);
    HDC memory = CreateCompatibleDC(dc);
    HBITMAP bitmap = CreateCompatibleBitmap(dc, canvas.right, canvas.bottom);
    HGDIOBJ old = SelectObject(memory, bitmap);
    DrawInterface(memory, canvas, app);
    BitBlt(dc, 0, 0, canvas.right, canvas.bottom, memory, 0, 0, SRCCOPY);
    SelectObject(memory, old);
    DeleteObject(bitmap);
    DeleteDC(memory);
    EndPaint(window, &paint);
}

void TrySolve(App& app) {
    if (!IsValidBoard(app.board)) {
        app.status = L"Resolve the highlighted clashes first.";
        UpdateConflicts(app);
        return;
    }
    int candidate[9][9]{};
    std::copy(&app.board[0][0], &app.board[0][0] + 81, &candidate[0][0]);
    if (!Solve(candidate)) {
        app.status = L"This board has no valid solution.";
        return;
    }
    std::copy(&candidate[0][0], &candidate[0][0] + 81, &app.board[0][0]);
    app.status = L"Solved. Every move checks out.";
    UpdateConflicts(app);
}

void MoveSelection(App& app, int rowDelta, int colDelta) {
    app.selectedRow = (app.selectedRow + rowDelta + 9) % 9;
    app.selectedCol = (app.selectedCol + colDelta + 9) % 9;
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* app = reinterpret_cast<App*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    switch (message) {
    case WM_NCCREATE:
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams));
        return TRUE;
    case WM_PAINT:
        if (app) {
            Paint(window, *app);
            return 0;
        }
        break;
    case WM_LBUTTONDOWN:
        if (app) {
            const int x = static_cast<short>(LOWORD(lParam));
            const int y = static_cast<short>(HIWORD(lParam));
            const RECT solve = MakeRect(722, 304, 1101, 360);
            const RECT reset = MakeRect(722, 374, 903, 426);
            const RECT next = MakeRect(920, 374, 1101, 426);
            if (Contains(MakeRect(kBoardLeft, kBoardTop, kBoardLeft + kBoardSize, kBoardTop + kBoardSize), x, y)) {
                app->selectedRow = (y - kBoardTop) / kCellSize;
                app->selectedCol = (x - kBoardLeft) / kCellSize;
            } else if (Contains(solve, x, y)) {
                TrySolve(*app);
            } else if (Contains(reset, x, y)) {
                ClearEntries(*app);
            } else if (Contains(next, x, y)) {
                LoadPuzzle(*app, app->puzzleIndex + 1);
            }
            SetFocus(window);
            InvalidateRect(window, nullptr, FALSE);
            return 0;
        }
        break;
    case WM_KEYDOWN:
        if (app) {
            bool redraw = true;
            if (wParam >= '1' && wParam <= '9') {
                EnterValue(*app, static_cast<int>(wParam - '0'));
            } else if (wParam >= VK_NUMPAD1 && wParam <= VK_NUMPAD9) {
                EnterValue(*app, static_cast<int>(wParam - VK_NUMPAD0));
            } else if (wParam == '0' || wParam == VK_NUMPAD0 || wParam == VK_BACK || wParam == VK_DELETE) {
                EnterValue(*app, 0);
            } else if (wParam == VK_LEFT) {
                MoveSelection(*app, 0, -1);
            } else if (wParam == VK_RIGHT) {
                MoveSelection(*app, 0, 1);
            } else if (wParam == VK_UP) {
                MoveSelection(*app, -1, 0);
            } else if (wParam == VK_DOWN) {
                MoveSelection(*app, 1, 0);
            } else if (wParam == VK_RETURN) {
                TrySolve(*app);
            } else if (wParam == 'N') {
                LoadPuzzle(*app, app->puzzleIndex + 1);
            } else if (wParam == 'R') {
                ClearEntries(*app);
            } else {
                redraw = false;
            }
            if (redraw) {
                InvalidateRect(window, nullptr, FALSE);
                return 0;
            }
        }
        break;
    case WM_GETMINMAXINFO:
        reinterpret_cast<MINMAXINFO*>(lParam)->ptMinTrackSize = POINT{1160, 860};
        reinterpret_cast<MINMAXINFO*>(lParam)->ptMaxTrackSize = POINT{1160, 860};
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int show) {
    App app;
    LoadPuzzle(app, 0);
    const wchar_t className[] = L"SudokuStudioWindow";
    WNDCLASSW windowClass{};
    windowClass.hInstance = instance;
    windowClass.lpszClassName = className;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassW(&windowClass);

    HWND window = CreateWindowExW(0, className, L"Sudoku Studio", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 1160, 860, nullptr, nullptr, instance, &app);
    if (!window) {
        return 0;
    }
    ShowWindow(window, show);
    UpdateWindow(window);
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
