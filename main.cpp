#include <iostream>
#include <istream>
#include <fstream>
#include <string>
#include <algorithm>
#include <sstream>
#include <vector>
#include <map>


typedef enum {
	input_arg,
	output_arg,
	check_arg
} enum_string_values;

static std::map<std::string, enum_string_values> g_enum_mapped_strings;

void init_input_map()
{
	g_enum_mapped_strings["-i"] = input_arg;
	g_enum_mapped_strings["-o"] = output_arg;
	g_enum_mapped_strings["--check"] = check_arg;
}

class SudokuSolver {

public:
	SudokuSolver(const bool check_flag_copy) :
		sudoku_field(height, std::vector<size_t>(width)),
		sudoku_occurence_field(9, std::vector<std::vector<bool>>(2, std::vector<bool>(9, false))),
		sudoku_candidate_field(3, std::vector<std::vector<bool>>(3, std::vector<bool>(9, false))),
		output_string(""),
		check_flag(check_flag_copy)
	{}

	~SudokuSolver() {}

	bool success = true;
	
	void update_occurence_field(const size_t cell_value, const size_t line_position, const bool update_value);
	void reset_sudoku_fields();
	bool check_value_occurence(const size_t cell_value, const size_t line_position);

	void parse_solution(const bool result);
	bool solve_sudoku(size_t position);

	friend std::istream& operator>>(std::istream& is, SudokuSolver& sudoku);
	friend std::ostream& operator<<(std::ostream& os, SudokuSolver& sudoku);

private:
	const size_t height = 9;
	const size_t width = 9;
	const size_t sudoku_length = height * width;
	const bool check_flag;
	std::ostringstream output_string;
	std::vector<std::vector<size_t>> sudoku_field;
	// Sudoku occurrence field is a 3D vector consisting of boolean information on whether a value exists in a column or row already
	// The format is as follow: vector[cell_value][is_row][position], where is_row is 1 if we're parsing a row position
	std::vector<std::vector<std::vector<bool>>> sudoku_occurence_field;
	std::vector<std::vector<std::vector<bool>>> sudoku_candidate_field;
};

void SudokuSolver::parse_solution(const bool result)
{
	if (result) 
	{
		for (size_t row = 0; row < height; row++)
			for (size_t column = 0; column < width; column++)
				output_string << sudoku_field[row][column];
	}

	output_string << "\n";

	
}

// line_position represents a flattened Sudoku format (1D 9*9). By default it updates to true.
void SudokuSolver::update_occurence_field(const size_t cell_value, const size_t line_position, const bool update_value = true)
{
	sudoku_occurence_field[cell_value - (size_t)(1)][0][line_position % 9] = update_value;
	sudoku_occurence_field[cell_value - (size_t)(1)][1][line_position / 9] = update_value;
	sudoku_candidate_field[line_position / (3 * height)][(line_position % 9) / 3][cell_value - (size_t)(1)] = update_value;
}

void SudokuSolver::reset_sudoku_fields()
{
	for (int cell_value = 0; cell_value < 9; cell_value++)
		for (int vector = 0; vector < 2; vector++)
			for (int occurence = 0; occurence < 9; occurence++)
				sudoku_occurence_field[cell_value][vector][occurence] = false;

	for (size_t row_box = 0; row_box < width / 3; row_box++)
		for (size_t column_box = 0; column_box < height / 3; column_box++) {
			for (size_t value = 1; value <= 9; value++)
				sudoku_candidate_field[row_box][column_box][value - 1] = false;
		}
}

// Returns true if value already exists in the row or column or box
bool SudokuSolver::check_value_occurence(const size_t cell_value, const size_t line_position)
{

	return
		(sudoku_occurence_field[cell_value - 1][0][line_position % width] || sudoku_occurence_field[cell_value - 1][1][line_position / height])
		||
		sudoku_candidate_field[line_position / (3 * height)][(line_position % 9) / 3][cell_value - (size_t)(1)];

}

bool SudokuSolver::solve_sudoku(size_t position)
{
	for (position; position < sudoku_length; position++) {
		if (sudoku_field[position / 9][position % 9] == 0) {
			for (int value = 1; value <= 9; value++)
				if (!check_value_occurence(value, position)) {
					update_occurence_field(value, position);
					sudoku_field[position / 9][position % 9] = value;
					if (solve_sudoku(position + 1))
						return true;
					update_occurence_field(value, position, false);
					sudoku_field[position / 9][position % 9] = 0;
				}
			return false;
		}
	}
	return true;
}

std::istream& operator>>(std::istream& is, SudokuSolver& sudoku) {

	std::string line("");

	while (std::getline(is, line)) {

		bool solvable = true;

		// The check_flag here serves no purpose yet. Will be used if I decide to implement bonus part
		// TODO: Implement checker
		if (line.length() != sudoku.sudoku_length && !sudoku.check_flag) {
			std::cout << "Incorrect line length" << std::endl;
			sudoku.success = false;
			is.setstate(std::ios::failbit);
			return is;
		}
		
		for (size_t position = 0; position < sudoku.sudoku_length * (size_t)(!sudoku.check_flag); position++) {
			char current_char = line[position];
			int char_int_representation = current_char - '0';

			if ('1' <= current_char && current_char <= '9')
				// Check if number exists in row and column
				if (!sudoku.check_value_occurence(char_int_representation, position))
				{
					sudoku.sudoku_field[position / 9][position % 9] = char_int_representation;
					sudoku.update_occurence_field(char_int_representation, position);
				}
				else 
				{
					solvable = false;
					break;
				}
			else
			if (current_char == '0' || current_char == '.')
				sudoku.sudoku_field[position / 9][position % 9] = 0;
			else
			{
				std::cout << "Illegal character encountered" << std::endl;
				is.setstate(std::ios::failbit);
				sudoku.success = false;
				return is;
			}
		}

		if (!solvable) {
			sudoku.parse_solution(false);
			sudoku.reset_sudoku_fields();
			continue;
		}

		if (!sudoku.check_flag) {
			size_t first_empty_cell;
			if ((first_empty_cell = line.find_first_of(".0")) == std::string::npos)
				sudoku.parse_solution(true);
			else
				sudoku.parse_solution(sudoku.solve_sudoku(first_empty_cell));

			sudoku.reset_sudoku_fields();
		}
	}

	is.setstate(std::ios::goodbit);
	return is;

}

std::ostream& operator<<(std::ostream& os, SudokuSolver& sudoku) {
	os << sudoku.output_string.str();
	return os;
}

/*bool io_sudoku_parser(std::istream& input_stream, std::ostream& output_stream, bool check_flag) {

	SudokuSolver sudoku(check_flag);

	//input_stream >> sudoku;
	if (!(input_stream >> sudoku))
		return false;

	if (!(output_stream << sudoku))
		return false;

	return true;

}*/

int main(int argc, char* argv[]) {

	init_input_map();

	std::vector<std::string> args(argv + 1, argv + argc);
	std::istream* input_stream = &std::cin;
	std::ifstream input_file;
	std::ostream* output_stream = &std::cout;
	std::ofstream output_file;
	bool check_flag = false;

	if (argc % 2 == 0) {
		std::cout << "Invalid input parameters\n";
		return EXIT_FAILURE;
	}

	for (auto argument = args.begin(); argument != args.end(); argument++) {
		switch (g_enum_mapped_strings[*argument]) {

			case check_arg:
				check_flag = true;
				[[fallthrough]];

			case input_arg:
				input_file.open(*(++argument));
				if (!input_file.is_open()) {
					std::cout << "File failed to open for reading." << std::endl;
					return EXIT_FAILURE;
				}
				input_stream = &input_file;
				break;

			case output_arg:
				output_file.open(*(++argument));
				if (!output_file.is_open()) {
					std::cout << "File failed to open for writing." << std::endl;
					return EXIT_FAILURE;
				}
				output_stream = &output_file;
				break;

			default:
				return EXIT_FAILURE;
		}
	}

	SudokuSolver sudoku(check_flag);

	*input_stream >> sudoku;
	if (!sudoku.success)
		return EXIT_FAILURE;

	if (!(*output_stream << sudoku))
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
	/*
	if (!io_sudoku_parser(*input_stream, *output_stream, check_flag))
		return EXIT_FAILURE;
	else
		return EXIT_SUCCESS;*/

}