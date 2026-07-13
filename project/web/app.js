const puzzles = [
  [[5,3,0,0,7,0,0,0,0],[6,0,0,1,9,5,0,0,0],[0,9,8,0,0,0,0,6,0],[8,0,0,0,6,0,0,0,3],[4,0,0,8,0,3,0,0,1],[7,0,0,0,2,0,0,0,6],[0,6,0,0,0,0,2,8,0],[0,0,0,4,1,9,0,0,5],[0,0,0,0,8,0,0,7,9]],
  [[0,0,0,2,6,0,7,0,1],[6,8,0,0,7,0,0,9,0],[1,9,0,0,0,4,5,0,0],[8,2,0,1,0,0,0,4,0],[0,0,4,6,0,2,9,0,0],[0,5,0,0,0,3,0,2,8],[0,0,9,3,0,0,0,7,4],[0,4,0,0,5,0,0,3,6],[7,0,3,0,1,8,0,0,0]],
  [[0,2,0,6,0,8,0,0,0],[5,8,0,0,0,9,7,0,0],[0,0,0,0,4,0,0,0,0],[3,7,0,0,0,0,5,0,0],[6,0,0,0,0,0,0,0,4],[0,0,8,0,0,0,0,1,3],[0,0,0,0,2,0,0,0,0],[0,0,9,8,0,0,0,3,6],[0,0,0,3,0,6,0,9,0]]
];

let puzzleIndex = 0;
let board = [];
let clues = [];
let selected = [0, 0];
let status = 'Ready when you are.';

const boardElement = document.querySelector('#board');
const statusElement = document.querySelector('#status');
const numbersElement = document.querySelector('#numbers');

function copyGrid(grid) {
  return grid.map((row) => [...row]);
}

function safe(grid, row, col, value) {
  for (let index = 0; index < 9; index += 1) {
    if (index !== col && grid[row][index] === value) return false;
    if (index !== row && grid[index][col] === value) return false;
  }
  const startRow = row - row % 3;
  const startCol = col - col % 3;
  for (let r = startRow; r < startRow + 3; r += 1) {
    for (let c = startCol; c < startCol + 3; c += 1) {
      if ((r !== row || c !== col) && grid[r][c] === value) return false;
    }
  }
  return true;
}

function valid(grid) {
  return grid.every((row, r) => row.every((value, c) => value >= 0 && value <= 9 && (!value || safe(grid, r, c, value))));
}

function options(grid, row, col) {
  return Array.from({ length: 9 }, (_, index) => index + 1).filter((value) => safe(grid, row, col, value));
}

function solve(grid) {
  let best = null;
  for (let row = 0; row < 9; row += 1) {
    for (let col = 0; col < 9; col += 1) {
      if (grid[row][col]) continue;
      const values = options(grid, row, col);
      if (!values.length) return false;
      if (!best || values.length < best.values.length) best = { row, col, values };
    }
  }
  if (!best) return true;
  for (const value of best.values) {
    grid[best.row][best.col] = value;
    if (solve(grid)) return true;
    grid[best.row][best.col] = 0;
  }
  return false;
}

function conflicts() {
  return board.map((row, r) => row.map((value, c) => Boolean(value) && !safe(board, r, c, value)));
}

function setStatus(next) {
  status = next;
  statusElement.textContent = status;
}

function render() {
  const bad = conflicts();
  const selectedValue = board[selected[0]][selected[1]];
  boardElement.innerHTML = '';
  board.forEach((row, r) => row.forEach((value, c) => {
    const button = document.createElement('button');
    button.type = 'button';
    button.className = 'cell';
    button.textContent = value || '';
    button.setAttribute('role', 'gridcell');
    button.setAttribute('aria-label', `Row ${r + 1}, column ${c + 1}${value ? `, ${value}` : ', empty'}`);
    if (clues[r][c]) button.classList.add('given');
    if (bad[r][c]) button.classList.add('conflict');
    if (r === selected[0] || c === selected[1] || (Math.floor(r / 3) === Math.floor(selected[0] / 3) && Math.floor(c / 3) === Math.floor(selected[1] / 3))) button.classList.add('related');
    if (selectedValue && value === selectedValue) button.classList.add('match');
    if (r === selected[0] && c === selected[1]) button.classList.add('active');
    button.addEventListener('click', () => {
      selected = [r, c];
      render();
    });
    boardElement.append(button);
  }));
  statusElement.textContent = status;
}

function load(index) {
  puzzleIndex = index % puzzles.length;
  board = copyGrid(puzzles[puzzleIndex]);
  clues = copyGrid(puzzles[puzzleIndex]);
  selected = [0, 0];
  setStatus('Fresh board loaded. Find its rhythm.');
  render();
}

function place(value) {
  const [row, col] = selected;
  if (clues[row][col]) {
    setStatus('That number is part of the original board.');
  } else {
    board[row][col] = value;
    const bad = conflicts();
    if (bad[row][col]) setStatus('That value clashes with the board.');
    else setStatus(value ? 'Value placed.' : 'Cell cleared.');
  }
  render();
}

function reset() {
  board = copyGrid(clues);
  setStatus('Editable cells cleared.');
  render();
}

function solveBoard() {
  if (!valid(board)) {
    setStatus('Resolve the highlighted clashes first.');
  } else {
    const candidate = copyGrid(board);
    if (solve(candidate)) {
      board = candidate;
      setStatus('Solved. Every move checks out.');
    } else {
      setStatus('This board has no valid solution.');
    }
  }
  render();
}

for (let value = 1; value <= 9; value += 1) {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'number';
  button.textContent = value;
  button.addEventListener('click', () => place(value));
  numbersElement.append(button);
}
const clearButton = document.createElement('button');
clearButton.type = 'button';
clearButton.className = 'number clear';
clearButton.textContent = 'Clear';
clearButton.addEventListener('click', () => place(0));
numbersElement.append(clearButton);

document.querySelector('#solve').addEventListener('click', solveBoard);
document.querySelector('#reset').addEventListener('click', reset);
document.querySelector('#next').addEventListener('click', () => load(puzzleIndex + 1));

window.addEventListener('keydown', (event) => {
  const key = event.key;
  if (/^[1-9]$/.test(key)) place(Number(key));
  else if (key === 'Backspace' || key === 'Delete' || key === '0') place(0);
  else if (key === 'ArrowLeft') selected = [selected[0], (selected[1] + 8) % 9];
  else if (key === 'ArrowRight') selected = [selected[0], (selected[1] + 1) % 9];
  else if (key === 'ArrowUp') selected = [(selected[0] + 8) % 9, selected[1]];
  else if (key === 'ArrowDown') selected = [(selected[0] + 1) % 9, selected[1]];
  else if (key === 'Enter') solveBoard();
  else if (key.toLowerCase() === 'n') load(puzzleIndex + 1);
  else if (key.toLowerCase() === 'r') reset();
  else return;
  event.preventDefault();
  render();
});

load(0);
