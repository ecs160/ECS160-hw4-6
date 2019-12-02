#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define LINE_MAX_CHAR 1024 // Valid file line will have max 1024 characters
#define MAX_FILE_LEN 20000 // Valid file will not exceed more than 20,000 lines

typedef struct Tweeter {
    char name[LINE_MAX_CHAR + 1];
    int count;
} Tweeter;

int findNameHeader(char *buffer, int buf_length, int* columns, bool* name_quoted);
int parseLine(char *buffer, int buf_length, int columns, int location, char tweeter[LINE_MAX_CHAR + 1], bool name_quoted);
int splitColumns(char *buffer, int buf_length, char ***column_array, int *column_array_size);
int addTweeter(Tweeter **tweeters, int *tweeters_count, char tweeter[LINE_MAX_CHAR + 1]);

int main(int argc, char* argv[]){
    FILE *csv_file;
    char *buffer = NULL;
    size_t buffer_size = 0;
    ssize_t num_char = 0;
    int line_count = 0;
    Tweeter *tweeters = NULL;
    int tweeters_count = 0;

    // Open the file
    if (argc == 2){
        csv_file = fopen(argv[1],"r");
        if (csv_file == NULL) {
            goto error;
        }
    }
    else{
        goto error;
    }

    // Read the header
    num_char = getline(&buffer,&buffer_size,csv_file);
    // If we read more than max num of chars or read nothing, then return invalid
    if (num_char > LINE_MAX_CHAR || num_char == -1){
        goto cf_error;
    }
    //printf("Read %zd characters\n", num_char);

    // Find where the name column is
    int location = 0, num_cols = 0;
    bool name_quotes = false;
    location = findNameHeader(buffer,num_char,&num_cols,&name_quotes);
    if (location == -1){
        goto cf_error;
    }
    //printf("Found name at: %d\n",location);

    // Read every line in the file
    num_char = getline(&buffer,&buffer_size,csv_file);
    while (num_char != -1) {
        int cur_return = 0;
        char cur_name[LINE_MAX_CHAR + 1];

        // Count the line
        line_count += 1;

        // If we read more than line or more lines than line limit, go to error
        if (num_char > LINE_MAX_CHAR || line_count > MAX_FILE_LEN - 1){
            goto cf_error;
        }

        // Read the line and find the tweeter name
        cur_return = parseLine(buffer, num_char, num_cols, location, cur_name, name_quotes);
        if (cur_return == -1){
            goto cf_error;
        }

        // Add the tweeter to our array
        cur_return = addTweeter(&tweeters,&tweeters_count,cur_name);
        if (cur_return == -1){
            goto cf_error;
        }

        num_char = getline(&buffer,&buffer_size,csv_file);
    }
    //printf("Lines read after header: %d\n", line_count);

    // Print all the top tweeters
    for (int i = 0; i < tweeters_count; i++){
        printf("%s: %d\n", tweeters[i].name, tweeters[i].count);
        if (i == 9){
            break;
        }
    }

    fclose(csv_file);
    free(tweeters);
    return 0;

    cf_error:
        fclose(csv_file);
    error:
        printf("Invalid Input Format\n");
        return 0;
}

int findNameHeader(char *buffer, int buf_length, int* columns, bool* name_quoted){
    /* Function takes in 4 input parameters:
     * @buffer: A string containing a csv line
     * @buf_length: An int length of @buffer
     * @columns: A pointer to an int which holds the number of columns counted
     * @name_quoted: A pointer to a bool which indicates if the name header was quoted
     * Function finds "name", and ensures it is the only occurrence
     * Upon success, it returns the column position of the word.
     * Upon any error, it returns -1 */

    char **column_array = NULL;
    int column_array_size = 0;
    int name_index = -1;

    // Split the columns and store in column array
    if (splitColumns(buffer,buf_length,&column_array,&column_array_size) == -1){
        return -1;
    }
    if (column_array == NULL){
        return -1;
    }

    // Set the columns
    (*columns) = column_array_size;

    for (int i = 0; i < column_array_size; i++){
        char *current = column_array[i];
        bool foundQuotes = false;

        if (current == NULL){
            return -1;
        }

        // Check for matching quotes
        if (current[0] == '"' && strlen(current) > 1 && current[strlen(current) - 1] == '"') {
            foundQuotes = true;
            for (int j = 1; j < (strlen(current) - 1); j++){
                current[j - 1] = current[j];
            }
            // Set last quote to \0
            current[strlen(current) - 2] = '\0';
        }

        // If we have a lone quotation mark at the start or end, return 0.
        if (current[0] == '"' || (strlen(current) > 0 && current[strlen(current) - 1] == '"')){
            return -1;
        }
        if (strcmp("name",current) == 0){
            if (foundQuotes){ (*name_quoted) = true;}
            name_index = i;
        }
    }

    // If we find any matching headers, return -1
    for (int i = 0; i < column_array_size; i++) {
        for (int j = i + 1; j < column_array_size; j++) {
            if (strcmp(column_array[i],column_array[j]) == 0){
                return -1;
            }
        }
    }

    for (int i = 0; i < column_array_size; i++){
        free(column_array[i]);
    }
    free(column_array);
    return name_index;
}

int splitColumns(char *buffer, int buf_length, char ***column_array, int *column_array_size){
    /* Function takes 4 arguments:
     * @buffer: A string containing the current line from the buffer
     * @buf_length: An int containing the length of @buffer
     * @column_array: A pointer to a string array which will contain the line in split form
     * @column_array_size: A pointer to an int which will contain the length of @column_array
     * Function gets the current line and splits it into an array where each column is a new index.
     * Returns 0 on success, -1 on failure.
     * */

    int column_count = 0;
    int cur_column_count = 0;
    bool start_col = true;
    char temp[LINE_MAX_CHAR + 1];

    for (int i = 0; i < buf_length; i++){
        if (start_col){
            column_count++;
            (*column_array) = (char**) realloc((*column_array), column_count * sizeof(char*));
            if (*column_array == NULL){
                return -1;
            }
            start_col = false;
        }
        if (buffer[i] != ','){
            temp[cur_column_count] = buffer[i];
            cur_column_count++;
            if (i == buf_length - 1){
                goto finalize;
            }
        }
        else {
            finalize:
            start_col = true;
            temp[cur_column_count] = '\0';
            (*column_array)[column_count - 1] = (char*) malloc((cur_column_count + 1) * sizeof(char));
            cur_column_count = 0;
            if ((*column_array)[column_count - 1] == NULL){
                return -1;
            }
            strcpy((*column_array)[column_count - 1],temp);
            
            // Clear temp
            for (int j = 0; j < LINE_MAX_CHAR + 1; j++){
                temp[j] = '\0';
            }
        }
    }
    (*column_array_size) = column_count;
    return 0;
}

int parseLine(char *buffer, int buf_length, int columns, int location, char tweeter[LINE_MAX_CHAR + 1], bool name_quoted){
    /* Function takes 6 input parameters:
     * @buffer: A string containing the contents of the line.
     * @buf_length: The length of the line string.
     * @columns: The number of columns obtained from the header.
     * @location: The index of the column which contains the tweeters
     * @tweeter: A pointer to a string which stores the tweeter name.
     * @name_quoted: A boolean indicating whether name was surrounded by quotes
     * Function goes through the line, making sure its formatting and column count match the requirements.
     * If the line is valid, it will extract the name in the @location, and store it in @tweeter.
     * Returns -1 on failure, 0 on success.*/

    char **column_array = NULL;
    int column_array_size = 0;

    // Split the columns and store in column array
    if (splitColumns(buffer,buf_length,&column_array,&column_array_size) == -1){
        return -1;
    }
    if(column_array == NULL){
        return -1;
    }

    // If we read more/less than @columns, return -1
    if (column_array_size != columns){
        //Free?
        return -1;
    }

    for (int i = 0; i < column_array_size; i++){
        char *current = column_array[i];
        bool foundQuotes = false;
        if (current == NULL){
            return -1;
        }

        // Check for matching quotes
        if (current[0] == '"' && strlen(current) > 1 && current[strlen(current) - 1] == '"') {
            foundQuotes = true;
            for (int j = 1; j < (strlen(current) - 1); j++){
                current[j - 1] = current[j];
            }
            // Set last quote to \0
            current[strlen(current) - 2] = '\0';
        }

        // If we have a lone quotation mark at the start or end, return 0.
        if (current[0] == '"' || (strlen(current) > 0 && current[strlen(current) - 1] == '"')){
            return -1;
        }

        // If we are at the tweeter location, store the tweeter name
        if (location == i){
            if (foundQuotes != name_quoted){
                return -1;
            }
            strcpy(tweeter,current);
        }
    }
    for (int i = 0; i < column_array_size; i++){
        free(column_array[i]);
    }
    free(column_array);
    return 0;
}

int addTweeter(Tweeter **tweeters, int *tweeters_count, char tweeter[LINE_MAX_CHAR + 1]){
    /* Function takes 3 input parameters:
     * @tweeters: A pointer to an array of Tweeter objects which holds all the tweeters and their tweet counts
     * @tweeters_count: A pointer to an int that holds the length of the @tweeters array
     * @tweeter: A string which holds the current tweeter to add.
     * Function will go through @tweeters, trying to find a match with @tweeter.
     * If a match is found, then the entry.count is incremented.
     * If a match is not found, then a new entry is created in @tweeters[@tweeters_count], with a .count of 1.
     * @tweeters_count is then incremented to reflect the new size
     * Returns 0 on success. -1 on failure.*/

    bool foundMatch = false;
    int added_index = 0;

    // Try to find a matching @tweeter in @tweeters
    for (int i = 0; i < (*tweeters_count); i++){
        // If we find a tweeter matching the current name, then add 1 to the count
        if (strcmp((*tweeters)[i].name,tweeter) == 0){
            (*tweeters)[i].count += 1;
            foundMatch = true;
            added_index = i;
            break;
        }
    }
    if (!foundMatch){
        // Allocate memory for new tweeter and add to the array
        (*tweeters_count) += 1;
        (*tweeters) = (Tweeter *) realloc((*tweeters),sizeof(Tweeter) * (*tweeters_count));
        if (*tweeters == NULL){
            return -1;
        }
        strcpy((*tweeters)[(*tweeters_count) - 1].name,tweeter);
        (*tweeters)[(*tweeters_count) - 1].count = 1;

        added_index = (*tweeters_count) - 1;
    }

    // Sort the added_index to make sure array is highest to lowest
    for (int i = added_index; i >= 0; i--){
        // Swap if element to the left is smaller than current element
        if (i > 0 && (*tweeters)[i].count > (*tweeters)[i - 1].count){
            char temp[LINE_MAX_CHAR + 1];
            int temp_num = 0;
            strcpy(temp,(*tweeters)[i - 1].name);
            strcpy((*tweeters)[i - 1].name,(*tweeters)[i].name);
            strcpy((*tweeters)[i].name,temp);
            temp_num = (*tweeters)[i - 1].count;
            (*tweeters)[i - 1].count = (*tweeters)[i].count;
            (*tweeters)[i].count = temp_num;
        }
        else{
            return 0;
        }
    }

    return 0;
}


