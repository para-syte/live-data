// before compiling run,
//  set -a
//  source .env
//  set +a
// to compile g++ -std=c++20 main.cpp -o build -lcurl
// to-do: output with respective data, things like symbols, times, volume, and price for now, eventually add tui with live data.

//want to eventually add socket programming for the live data part.

#include <cstdlib>
#include <cstring>
#include <curl/curl.h>
#include <curl/easy.h>
#include <stdio.h>
#include <iostream>
#include "json.hpp"

using json = nlohmann::json;

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

// function to insert ticker into the url
std::string stockSymbol(std::string symbol) {
  std::string endLink =
      "&symbol=&interval=30min&format=JSON&type=stock&timezone=exchange&start_"
      "date=2026-09-08T08:30:00&end_date=2026-09-08T15:00:00";
  endLink.insert(8, symbol);
  return endLink;
}

void httpsSetGet(CURL *curl, CURLcode result, struct MemoryStruct &chunk, std::string fullLink) {
  curl_easy_setopt(
      curl, CURLOPT_URL,
      fullLink.c_str()); // sets https destination
  curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L); // establishes the GET request
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, wcb); //calls the wcb function
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
  result = curl_easy_perform(curl); //performs the GET

  if (result != CURLE_OK) {
    std::cerr << "error:\n" << curl_easy_strerror(result);
    exit(-1);
  }

}

void tableOutput(std::string ticker) {
  std::cout << std::format("{:<8} {:<15} {:<15} {:<15} {:<15} {:<15} {:<15}\n",
                           "Ticker", "Open price", "Open volume", "Midday price", "Midday volume", "Close price", "Close volume");
  std::cout << std::string(101, '-') << "\n";
  std::cout << ticker;
  std::cout << std::endl;
}

int main(int argc, char *argv[]) {
  const char *envVariable = std::getenv("TWELVEDATA_API_KEY");
  if (envVariable == nullptr) {
    std::cerr << "api key is null\n";
    exit(-1);
  }
  std::string firstLink = "https://api.twelvedata.com/time_series?apikey=";
  std::string apiString = envVariable;
  
  CURL *curl = curl_easy_init();
  CURLcode result;

  if (curl == NULL) {
    std::cerr << "http request failed.\n";
    return -1;
  }
  
  struct MemoryStruct chunk;
  chunk.memory = static_cast<char *>(malloc(1));
  chunk.size = {0};

  std::string endLink = stockSymbol("AAPL");
  std::string fullLink = firstLink + apiString + endLink;

  httpsSetGet(curl, result, chunk, fullLink);

  json data = json::parse(chunk.memory); // parsing the json data directly

  // std::cout << data << std::endl;

  std::string ticker = data["meta"]["symbol"];

  // std::cout << data["meta"]["symbol"] << std::endl;
  // std::cout << data["values"][0]["close"] << std::endl;

  tableOutput(ticker);

  free(chunk.memory);
  chunk.memory = NULL;
  curl_easy_cleanup(curl);
     
  return 0;
}
