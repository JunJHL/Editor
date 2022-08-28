#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stack>
#include <iomanip>

void move_crs_left_right(int &cur, std::string command);
void move_crs_up_down(int &cur, std::vector<std::string> file, std::string command, int& top, int& bottom);
void display_file(const std::vector<std::string>& file, int col_crs_pos, int row_crs_pos, int top);
void insert_line(std::vector<std::string>& file, std::string line_insert, int row_crs_pos, int col_crs_pos);
void undo_redo_line(std::vector<std::string> &file, std::stack<std::vector<std::string> > &undo_his, std::stack<std::vector<std::string> > &redo_his, std::string command, int& dist_from_save);
void save_file(std::ofstream& file_dst, const std::vector<std::string>& file_source);
void process_command(std::string command, int& line_crs_pos, int& char_crs_pos, std::vector<std::string>& file, std::stack<std::vector<std::string> >& undo_his, std::stack<std::vector<std::string> >& redo_his, int& top, int& bottom, int& dist_from_save);
std::vector<std::string> read_input(std::fstream& fp);
bool file_input_valid(std::string filename, const std::vector<std::string>& contents);

int main(int argc, char* argv[]) {
    int line_crs_pos = 1;
    int char_crs_pos = 1;
    std::string command = "";
    std::string last_command = "";
    std::stack<std::vector<std::string> > undo_his;   
    std::stack<std::vector<std::string> > redo_his;
    std::string consent = "n";
    int dist_from_save = 0;
    int top = 1;
    int down = 10;

    if(argc < 2) {
       std::cout << "Too few command-line arguments." << std::endl; 
       return 1;
    }else if(argc > 2) {
       std::cout << "Too many command-line arguments." << std::endl;
       return 1;
    }

    std::string filename = argv[1]; 
    std::fstream fp;
    fp.open(filename);
    
    if(!fp.is_open()) {
       std::cout << "Failed to open file: " << filename << std::endl;
       return 2;
    }

    std::vector<std::string> file = read_input(fp); 
    if(!file_input_valid(filename, file)) {
        return 2;
    }

    do{   
        display_file(file, char_crs_pos, line_crs_pos, top);
        std::cout << "Enter command: ";
        std::getline(std::cin, command);
        if(command == "") {  
            if(last_command == "") {
                std::cout << std::endl << "No previous command." << std::endl;
            }else {
                process_command(last_command, line_crs_pos, char_crs_pos, file, undo_his, redo_his, top, down, dist_from_save);
            }
        }else {
            process_command(command, line_crs_pos, char_crs_pos, file, undo_his, redo_his, top, down, dist_from_save);
        }
        last_command = command;
        std::cout << std::endl;

        if(command == "q" && dist_from_save != 0) {
            std::cout << std::endl << "You have unsaved changes." << std::endl;
            std::cout << "Are you sure you want to quit (y or n)?" << std::endl; 
            std::getline(std::cin, consent);                // don't use cin since cin does not put \n in the input, the \n stays in input buffer
        }else if(command == "q" && dist_from_save == 0) {
            break;
        }

    }while(command != "q" || consent != "y");    

    std::cout << "Goodbye!" << std::endl;
    fp.close();
    return 0;
}

void process_command(std::string command, int& line_crs_pos, int& char_crs_pos, std::vector<std::string>& file, std::stack<std::vector<std::string> >& undo_his, std::stack<std::vector<std::string> >& redo_his, int& top, int& bottom, int& dist_from_save) {
   std::vector<std::string> file_ori = file;

   if(command.find("save") != std::string::npos) {
        std::string saved_filename = command.substr(5, command.length() - 5);
        std::ofstream file_save;
        file_save.open(saved_filename);
        save_file(file_save, file);
        dist_from_save = 0;
        file_save.close();
    }else if(command.find("i") != std::string::npos) {
        undo_his.push(file_ori);
        insert_line(file, command.substr(2, command.length() - 2), line_crs_pos, char_crs_pos);
        ++dist_from_save;
        while(!redo_his.empty()) {
            redo_his.pop(); // pop everything in redo
        } 
    }else if(command.find("u") != std::string::npos || command.find("r") != std::string::npos) {
        undo_redo_line(file, undo_his, redo_his, command, dist_from_save);
    }else if(command.find("a") != std::string::npos || command.find("d") != std::string::npos) {
        move_crs_left_right(char_crs_pos, command);
    }else if(command.find("w") != std::string::npos || command.find("s") != std::string::npos) {
        move_crs_up_down(line_crs_pos, file, command, top, bottom);
    }
}

void undo_redo_line(std::vector<std::string> &file, std::stack<std::vector<std::string> > &undo_his, std::stack<std::vector<std::string> > &redo_his, std::string command, int& dist_from_save) {
    if(command.find("u") != std::string::npos && !undo_his.empty()) {
        redo_his.push(file);
        file = undo_his.top();
        undo_his.pop();
        --dist_from_save;
    }else if(command.find("r") != std::string::npos && !redo_his.empty()) {
        undo_his.push(file);
        file = redo_his.top();
        redo_his.pop();
        ++dist_from_save;
    }else if(command.find("u") != std::string::npos && undo_his.empty()) {
        std::cout << std::endl << "Cannot undo." << std::endl;
    }else if(command.find("r") != std::string::npos && redo_his.empty()) {
        std::cout << std::endl << "Cannot redo." << std::endl;
    }
}

void insert_line(std::vector<std::string>& file, std::string line_insert, int row_crs_pos, int col_crs_pos) {
    int row_insert = row_crs_pos-1;
    int col_insert = col_crs_pos-1;
    std::string str = " ";

    for(int i = 0; i < (int)line_insert.length(); i++) {
        if(col_insert > 19) {
            if(row_insert >= (int) file.size()-1) {    // add row
                file.push_back(str);
            }
            ++row_insert;
            col_insert %= 20;
        }
        
        while(col_insert > (int)file.at(row_insert).length()-1) {    // add columns
            file.at(row_insert).push_back(' ');
        }
        file.at(row_insert).at(col_insert) = line_insert.at(i);
        ++col_insert;
    }
}

void move_crs_up_down(int &cur, std::vector<std::string> file, std::string command, int& top, int& bottom) {
    int num_op_run = 1;

    if(command.length() > 1) {
        num_op_run = std::stoi(command.substr(command.find(" ")+1, command.length()));
    }

    if(command.find("w") != std::string::npos) {
        if(cur - num_op_run < 1) {
            cur = 1;
            top = cur;
        }else if(cur == top && cur - num_op_run >= 1) {
            top -= num_op_run;
            cur -= num_op_run;
            bottom = top + 9;
        }else if(cur != top && cur - num_op_run >= 1){
            cur -= num_op_run;
        }
        
    }else if(command.find("s") != std::string::npos) {
        if((int)file.size() < 10) {
            if(cur + num_op_run >= (int) file.size()) {
                cur = (int)file.size();
                top = cur;
            }else if(cur + num_op_run > (int) file.size() && cur != (int) file.size()) {
                cur = (int)file.size();
                top += num_op_run - 1;
            }else if(cur == (int) file.size() && cur != top) {
                top += num_op_run;
                bottom = top + 9;
            }else if(cur == (int) file.size() && cur == top) {
            }else {
                cur += num_op_run;
            }
        }else if((int)file.size() == 10) {
            if(cur + num_op_run > top + 9 && cur != bottom) {
                cur = (int)file.size();
                top = cur;
            }else if(cur == bottom){
                top += num_op_run;
                bottom = top + 9;
            }else if(cur == (int) file.size() && cur != top) {
                top += num_op_run;
                bottom = top + 9;
            }else if(cur == (int) file.size() && cur == top) {
            }else {
                cur += num_op_run;
            }
        }else {
            if(top + num_op_run > (int)file.size() && cur != bottom) {
                cur = (int)file.size();
                top = cur;
            }else if(cur != (int) file.size() && cur == bottom){
                top += num_op_run;
                cur += num_op_run;
                bottom = top + 9;
            }else if(cur == (int) file.size() && cur != top) {
                top += num_op_run;
                bottom = top + 9;
            }else if(cur == (int) file.size() && cur == top) {
            }else {
                cur += num_op_run;
            }
        }
    }
}

void move_crs_left_right(int &cur, std::string command) {
    int num_op_run = 1;
    if(command.length() > 1) {
        num_op_run = std::stoi(command.substr(command.find(" ")+1, command.length()));
        
    }

    if(command.find("a") != std::string::npos) {
        if(cur-num_op_run <= 1) {
            cur = 1;
        }else {
            cur -= num_op_run;
        }
    }else if(command.find("d") != std::string::npos) {
        if(cur+num_op_run >= 20) {
            cur = 20;
        }else {
            cur += num_op_run;
        }
    }
}

void save_file(std::ofstream& file_dst, const std::vector<std::string>& file_source) {
    for(int i = 0; i < (int)file_source.size(); i++) {
        file_dst << file_source.at(i) << std::endl;
    }
}

void display_file(const std::vector<std::string>& file, int col_crs_pos, int row_crs_pos, int top) {
    std::cout << std::setw(col_crs_pos+5) << "*" << std::endl;
    std::cout << std::setw(6);
    for(int i = 1; i <= 20; i++) {            
        std::cout << i % 10;
    }
    std::cout << std::endl;

    int line_dis_pos = 1;
    for(int i = 0; i < 10; i++) {
        
        line_dis_pos = top + i;

        if(line_dis_pos == row_crs_pos) {
            std::cout << "*" << std::setw(3) << line_dis_pos;
        }else {
            std::cout << std::setw(4) << line_dis_pos;
        }   
        
        if((int)file.size() >= line_dis_pos) {
            std::cout << "|" << file.at(line_dis_pos-1);
        }
        std::cout << std::endl;
    }  
    std::cout << std::setw(6); 
    for(int i = 1; i <= 20; i++) {            
        std::cout << i % 10;
    }
    std::cout << std::endl;
}

std::vector<std::string> read_input(std::fstream& fp) {
    std::string line = "";
    std::vector<std::string> contents;

    while(std::getline(fp, line)) {
        contents.push_back(line);
    }

    return contents;
}

bool file_input_valid(std::string filename, const std::vector<std::string>& contents) {
    if(contents.size() > 30) {
        std::cout << "File " << filename << " has too many lines." << std::endl;
        return false;
    }
    for(const auto& line : contents) {
        if(line.size() > 20) {
            std::cout << "File " << filename << " has at least one too long line." << std::endl;
            return false;
        }
    }
    return true;
}