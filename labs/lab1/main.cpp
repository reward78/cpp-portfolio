#include <iostream>
#include <fstream>
#include <cstring>
const int kTamplateBufferSize = 1024;
char temp[kTamplateBufferSize];
const int kDataBuffeerSize = 101;
char dat[kDataBuffeerSize];
const int kMaxPairs = 1024;

struct Pair{
    char key[kDataBuffeerSize];
    char value[kDataBuffeerSize];
};

Pair datapair[kMaxPairs]; //создаем структуру для ключей/значений
int pair_count = 0; // подсчет пар
bool FindSpaces(char c[]){
    for(int i = 0;c[i] != '\0';++i){
        if(c[i] == ' ' or c[i] == '\t'){
            return true;
        }
    }
    return false;
}


// удаляем пробелы между ключами и значениями
void DeleteSpaces(char s[]){
    int i = 0;
    int j = 0;
    if(FindSpaces(s)){
        while(s[i] != '\0'){
            if(s[i] != ' ' and s[i] != '\t'){
                s[j] = s[i];
                j++;
            }
            ++i;
        }

        s[j] = '\0';
    }
}


// удаляем пробелы в ключах/значениях
void Trim(char z[]){
    int len = strlen(z);
    int start = 0;
    while(start < len and (z[start] == ' ' or z[start] == '\t' or z[start] == '\r' or z[start] == '\n')){
        start++;
    }

    int end = len - 1;

    while(end >= start and (z[end] == ' ' or z[end] == '\t' or z[end] == '\r' or z[end] == '\n')){
        end--;
    }

    int j = 0;

    for(int i = start; i <= end;i++){
        z[j++] = z[i];
    }

    z[j] = '\0';
}

//добавляем в структуру ключи/значения из файла
void FindKeyValue(const char b[]) {
    int i = 0;
    int k = 0;

    char key[kDataBuffeerSize];
    char value[kDataBuffeerSize];

    while(b[i] != '=' and b[i] != '\0'){
        key[k++] = b[i++];
    }

    key[k] = '\0';
    Trim(key);

    if(b[i] == '=') i++;
    
    
    k = 0;
    while(b[i] != '\0' and b[i] != '\n'){
        value[k++] = b[i++];
    }

    value[k] = '\0';
    Trim(value);

    for(int j = 0;j < pair_count;++j){
        if(strcmp(datapair[j].key, key) == 0){
            strcpy(datapair[j].value, value);
            return;
        }
    }
    strcpy(datapair[pair_count].key, key);
    strcpy(datapair[pair_count].value, value);
    pair_count++;
    }

void RememberPaths(int argc, char* argv[],const char*& templateFile,const char*& outputFile,const char*& dataFile){
    for(int i = 0;i < argc;++i){
        if(strcmp(argv[i], "-t") == 0 and i + 1 < argc){
            templateFile = argv[++i];
        }

        else if(strcmp(argv[i], "-d") == 0 and i + 1 < argc){
                dataFile = argv[++i];
        }

        else if(strcmp(argv[i], "-o") == 0 and i + 1 < argc){
            outputFile = argv[++i];
        }

        else if (strncmp(argv[i], "--template=", 11) == 0){
            templateFile = argv[i] + 11;
        }

        else if(strncmp(argv[i], "--data=", 7) == 0){
            dataFile = argv[i] + 7;
        }

        else if(strncmp(argv[i], "--output=", 9) == 0){
            outputFile = argv[i] + 9;
        }
    }
    
}



int main(int argc, char* argv[]){
    const char* dataFile = nullptr;
    const char* outputFile = nullptr;
    const char* templateFile = nullptr;

    RememberPaths(argc, argv, templateFile, outputFile, dataFile);
    
    if(!dataFile or !templateFile){
        return 2;
    }
    std::fstream file(templateFile);
    if(!file.is_open()){
        return 3;
    }


    std::fstream data(dataFile);
    if(!data.is_open()){
        return 3;
    }


    while(data.getline(dat, kDataBuffeerSize)){
        if(!(dat[0] == '#' or (dat[0] == '/' and dat[1] == '/'))){

        DeleteSpaces(dat);

        FindKeyValue(dat);
        
        }
    }
    bool miss_key = false;
    bool syntax_error = false;
    bool firststr = true;
    

    
    std::ofstream file_out;
    
    std::ostream* result1 = &std::cout;
    if(outputFile){
        file_out.open(outputFile);
        if(!file_out.is_open())
            return 3;
        result1 = &file_out;
        
    }

    while(file.getline(temp, kTamplateBufferSize)){
        char result[kTamplateBufferSize * 2];
        int result_index = 0;

        for (int i = 0;temp[i] != '\0';){
            if (temp[i] == '{' and temp[i + 1] == '{'){
                i += 2;
                char keys[101];
                int k = 0;
                while(temp[i] != '\0' and !(temp[i] == '}' and temp[i+1] == '}')){
                    keys[k++] = temp[i++];
                }

                keys[k] = '\0';
                
                if(temp[i] == '}' and temp[i + 1] == '}'){ 
                    i += 2;
                } else if (temp[i] != '}' and temp[i + 1] != '}'){
                    syntax_error = true;
                    break;
                }
                
                Trim(keys);

                bool found = false;
                for(int j = 0;j < pair_count;++j){
                    if(strcmp(datapair[j].key, keys) == 0){
                        strcpy(&result[result_index], datapair[j].value);
                        result_index += strlen(datapair[j].value);
                        found = true;
                        
                    }
                }
                if(!found){
                    miss_key = true;
                    result[result_index++] = '{';
                    result[result_index++] = '{';
                    strcpy(&result[result_index], keys);
                    result_index += strlen(keys);
                    result[result_index++] = '}';
                    result[result_index++] = '}';
                }
        }   else {
            result[result_index++] = temp[i++]; 
            }
        }
    result[result_index] = '\0';

    if(firststr){
        (*result1) << result;
        firststr = false;

            }
        else (*result1) << '\n' << result;
        
    }
    
    if(miss_key) { 
        return 1;
    }
    if(syntax_error){  
        return 4;
    }
    return 0;
}