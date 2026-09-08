// compile with -lcurl flag at the end of the command

#include <cstdlib>
#include <cstring>
#include <curl/curl.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "json.hpp"

using json = nlohmann::json;

namespace fs =std::filesystem;

const char *envVariable = std::getenv("API_KEY");

//struct with members which we use
struct MemoryStruct {
  char *memory;
  size_t size;
};

//function with parameters for data being read
size_t wcb(char *data, size_t size, size_t nmemeb, void *userdata) {
  auto real_size{size * nmemeb}; // size of data received

  //ptr pointing to chunk in main
  struct MemoryStruct *mem {
    (struct MemoryStruct *)userdata
  };
  
  //need to allocate memory of previous size + new size of the data + null terminator
  char *ptr =
      static_cast<char *>(realloc(mem->memory, mem->size + real_size + 1));

  if (!ptr) {
    return 0;
  }

  mem->memory = ptr;
  std::memcpy(&(mem->memory[mem->size]), data, real_size); //maps the new data into the correct index of the previous data 
  mem->size += real_size; //new size is basically previous data + new data
  mem->memory[mem->size] = 0; //

  return real_size;
}

int main(int argc, char *argv[]) {
  CURL *curl  = curl_easy_init();
  CURLcode result;
  
  std::string fileName = "output.json";
  if (fs::exists(fileName)) {
    fs::remove(fileName);
  }
  std::ofstream oFile(fileName);

  if (curl == NULL) {
    std::cerr << "http request failed.\n";
    return -1;
  }

  struct MemoryStruct chunk; //
  chunk.memory = static_cast<char*>(malloc(1)); //allocate the memory needed
  chunk.size = {0}; // setting size of data received back to 0

  if (envVariable == nullptr) {
    std::cerr << "api key not found\n";
    exit(-1);
  }

  std::string firstLink = "https://api.twelvedata.com/time_series?apikey=";
  std::string apiString = envVariable;
  std::string endLink = "&symbol=AAPL&interval=1min&format=JSON&timezone=America/Chicago&type=stock";
  std::string fullLink = firstLink + apiString + endLink;
  
  curl_easy_setopt(
      curl, CURLOPT_URL,
      fullLink.c_str()); // sets https destination
  curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L); // establishes the GET request
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, wcb); //calls the wcb function
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
  result = curl_easy_perform(curl); //performs the GET

  if (result != CURLE_OK) {
    std::cerr << "error:\n" << curl_easy_strerror(result);
    return -1;
  }

  oFile << chunk.memory; // write to file
  oFile.close(); // immediately close the file to be able to read from it
  std::ifstream f(
      "output.json"); // can only pass the file name as a string NOT an object
  json data = json::parse(f); // parsing the json file
  
 
  free(chunk.memory);
  chunk.memory = NULL;
  curl_easy_cleanup(curl);
  
  return 0;
}
