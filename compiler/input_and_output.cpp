#include "intput_and_output.hpp"
// hlhlsadjfka;fj;laskdj
// the std::ifstream cpp_source had better be set as a member variable, maybe.
namespace hebo
{
	std::string uint2string(unsigned int input)
	{
		std::stringstream trans;
		trans << input;
		return trans.str();
	}
	int string2int(const std::string& input)
	{
		std::stringstream trans;
		trans << input;
		int result;
		trans >> result;
		return result;
	}

	DFA::DFA(std::string source_file_name, int row_number_, int column_number_)
		:current_status(1), current_memory(0), row_number(row_number_), column_number(column_number_)
	{
		cpp_source.open(source_file_name);
		if (!cpp_source.is_open())
		{
			std::cerr << "Not open the source file." << std::endl;
		}
		matrix = new int*[row_number];
		for (int i = 0; i < row_number; i++)
		{
			matrix[i] = new int[column_number];
			for (int j = 0; j < column_number; j++)
			{
				matrix[i][j] = i * 10 + j;
			}
		}
	}

	DFA::DFA(DFA& input) :current_status(1)
	{
		row_number = input.row_number;
		column_number = input.column_number;
		matrix = new int*[row_number];
		for (int i = 0; i < row_number; i++)
		{
			matrix[i] = new int[column_number];
			for (int j = 0; j < column_number; j++)
			{
				matrix[i][j] = input.matrix[i][j];
			}
		}
		status_to_pattern = input.status_to_pattern;
	}

	DFA::DFA(std::string dfa_file_name, std::string source_file_name)
	{
		cpp_source.open(source_file_name, std::ios::in);
		std::ifstream dfa_file(dfa_file_name);
		if (!dfa_file.is_open() || !cpp_source.is_open())
		{
			std::cerr << "Not open files" << std::endl;
		}
		std::string temp_string;
		dfa_file >> temp_string >> row_number;
		dfa_file >> temp_string >> column_number;
		dfa_file >> temp_string >> init_status;
		dfa_file >> temp_string;
		matrix = new int*[row_number];
		for (int i = 0; i < row_number; i++)
		{
			matrix[i] = new int[column_number];
			for (int j = 0; j < column_number; j++)
			{
				dfa_file >> matrix[i][j];
			}
		}
		dfa_file >> temp_string;
		int end_status;
		std::string pattern;
		while (dfa_file >> end_status >> pattern)
		{
			status_to_pattern.insert(std::make_pair(end_status, pattern));
			if (pattern == "//")
			{
				line__note_ = end_status;
			}
			else if (pattern == "/*")
			{
				left__note_ = end_status;
			}
			else if (pattern == "*/")
			{
				right_note_ = end_status;
			}
		}
		current_status = init_status;
#ifdef DEBUG
		for (int i = 0; i < row_number; i++)
		{
			for (int j = 0; j < column_number; j++)
			{
				std::cout << matrix[i][j] << ' ';
			}
			std::cout << std::endl;
		}
#endif // DEBUG

	}

	DFA::~DFA()
	{
		for (int i = 0; i < row_number; i++)
		{
			delete[] matrix[i];
			matrix[i] = nullptr;
		}
		delete[] matrix;
		matrix = nullptr;
		status_to_pattern.clear();
		row_number = 0;
		column_number = 0;
		word_list.clear();
	}

	std::ostream& operator<<(std::ostream& my_out, const DFA& dfa)
	{
		for (int i = 0; i < dfa.row_number; i++)
		{
			for (int j = 0; j < dfa.column_number; j++)
			{
				my_out << dfa.matrix[i][j] << ' ';
			}
			my_out << std::endl;
		}
		return my_out;
	}

	/*

	-> means turn into

	input
	the current character
	the current string { member variable }
	the current status { member variable }
	output



	modify
	the current status -> the next status
	the current string -> the next string = the current string + the character { update member variable }
	word_list if dead status and new morpheme then append a (morpheme, value)
	output_sequence if dead status and new morpheme then append a unit
	*/

	//feed a single character from the source file to the dfa matrix and monitor the transition of the status
	auto DFA::feed(char ch) -> void
	{
		int next_status = matrix[current_status][ch];
		bool is_dead = (next_status == dead_status);
		bool is_line = (next_status == line__note_);
		bool is_left = (next_status == left__note_);
		if (!(is_dead || is_line || is_left))
		{
			current_string += ch;
			current_status = next_status;
		}
		else if (is_dead)
		{
			update_output_sequence();
			current_string = ch;
			current_status = matrix[init_status][ch];
		}
		else if (is_line)
		{
			deal_with_line_note();
		}
		else if (is_left)
		{
			deal_with_multiplied_note();
		}
	}

	void DFA::deal_with_multiplied_note()
	{
		current_status = init_status;
		char ch;
		while (1)
		{
			ch = cpp_source.get();
			current_status = matrix[current_status][ch];
			if (current_status == dead_status)
			{
				current_status = matrix[init_status][ch];
			}
			else if (current_status == right_note_)
			{
				break;
			}
		}
		current_status = init_status;
		current_string.clear();
	}


	void DFA::deal_with_line_note()
	{
		std::getline(cpp_source, std::string());
		current_string.clear();
		current_status = init_status;
	}

	void DFA::update_output_sequence()
	{
		LexicalUnit lexical_unit;
		lexical_unit.name = status_to_pattern[current_status];
		lexical_unit.morpheme = current_string;

		if (lexical_unit.name == "BLANK")
		{
			return;
		}
		else if (lexical_unit.name.empty())
		{
			lexical_unit.name = "WRONG INPUT";
		}
		output_sequence.push_back(lexical_unit);
		current_status = init_status;
		current_string.clear();
		current_string = std::string("1");
	}



	void DFA::run()
	{
		char ch;
		while ((ch = cpp_source.get()) != -1)
		{

			feed(ch);
		}
		feed(0);
		std::ofstream output("lexical analyzer.txt");
		print_output_sequence(output);
	}


	DFA& DFA::operator<<(std::ifstream& input)
	{
		char ch;
		while ((ch = input.get()) != -1)
		{
			std::cout << ch;
		}
		return *this;
	}

	void DFA::print_current_string()
	{
		std::cout << current_string << std::endl;
	}

	void DFA::print_output_sequence(std::ostream& output)
	{
		output.setf(std::ios::left);
		output
			<< std::setw(15) << "Name"
			<< std::setw(15) << "Morpheme"
			<< std::endl << std::endl;

		for (int i = 0; i < output_sequence.size(); i++)
		{
			output
				<< std::setw(15) << output_sequence[i].name
				<< std::setw(15) << output_sequence[i].morpheme
				<< std::endl;
		}
	}
}
