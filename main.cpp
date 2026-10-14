// before compiling run,
//  set -a
//  source .env
//  set +a
//  currently working on setting up ftxui -> socket programming for live data
// to do: fill table with relevant data, fix string with september date, make table take up full screen

// read more on thread safety

#include <cstdlib>
#include <cstring>
#include <string>
#include <format>
#include <curl/curl.h>
#include <curl/easy.h>
#include <ftxui/dom/flexbox_config.hpp>
#include <stdio.h>
#include <iostream>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/dom/table.hpp> 
#include <chrono>
#include <thread>
#include "json.hpp"

using json = nlohmann::json;
using namespace ftxui;

// memorystruct for memory and size
struct MemoryStruct {
  char *memory;
  size_t size;
};

// function with parameters for data being read
size_t wcb(char *data, size_t size, size_t nmemeb, void *userdata) {
  auto real_size{size * nmemeb}; // size of data received

  // ptr pointing to chunk in main
  struct MemoryStruct *mem {
    (struct MemoryStruct *)userdata
  };
  
  // need to allocate memory of previous size + new size of the data + null terminator
  char *ptr =
      static_cast<char *>(realloc(mem->memory, mem->size + real_size + 1));

  if (!ptr) {
    return 0;
  }

  mem->memory = ptr;
  std::memcpy(&(mem->memory[mem->size]), data, real_size); // maps the new data into the correct index of the previous data 
  mem->size += real_size; // new size is basically previous data + new data
  mem->memory[mem->size] = 0; //

  return real_size;
}

// function to insert ticker into the url
std::string stockSymbol(std::string symbol) {
  // time_point used to fetch current system date
  auto now = std::chrono::system_clock::now();
  std::string date = std::format(":%Y-%m-%d", now);
  std::string endLink =
      "&symbol=&interval=30min&format=JSON&type=stock&timezone=exchange&start_"
      "date=2026-09-T08:30:00&end_date=2026-09-08T15:00:00";
  // not the most ideal way to insert the date but this will do for now
  endLink.insert(105, date);
  endLink.insert(76, date);  
  endLink.insert(8, symbol);
  
  return endLink;
}

// function to establish connection
void httpsSetGet(CURL *curl, CURLcode result, struct MemoryStruct &chunk, std::string fullLink) {
  curl_easy_setopt(
      curl, CURLOPT_URL,
      fullLink.c_str()); // sets https destination
  curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L); // establishes the GET request
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, wcb); //calls the wcb function
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
  result = curl_easy_perform(curl); //performs the GET

  if (result != CURLE_OK) {
    std::cerr << "error:" << curl_easy_strerror(result) << std::endl;
    exit(-1);
  }
}

void createUI(std::string ticker, json data) {
  // creates screen terminal/fullscreen
  auto screen = ScreenInteractive::Fullscreen();
  int counter = 0;
     
  /* renderer component for data,
    [&] used to reference outside of lambda variables */
  auto renderer = Renderer([&] {
    // currently putting dummy values, NEED TO FIX
    auto table = Table({
        {"TICKER", "CURRENT PRICE", "VOLUME", "OPEN", "HIGH", "LOW"},
        {ticker, data["values"][0]["close"], data["values"][0]["volume"], "10", "Dalvik 1.4.0"},
        {"4.0", "Ice Cream Sandwich", "October 19 2011", "15", "Dalvik"},
        {"4.1", "Jelly Bean", "July 9 2012", "16", "Dalvik"},
        {"4.2", "Jelly Bean", "November 13 2012", "17", "Dalvik"},
        {"4.3", "Jelly Bean", "July 24 2013", "18", "Dalvik"},
        {"4.4", "KitKat", "October 31 2013", "19", "Dalvik and ART"},
        {"5.0", "Lollipop", "November 3 2014", "21", "ART"},
        {"5.1", "Lollipop", "March 9 2015", "22", "ART"},
        {"6.0", "Marshmallow", "October 5 2015", "23", "ART"},
        {"7.0", "Nougat", "August 22 2016", "24", "ART"},
        {"7.1", "Nougat", "October 4 2016", "25", "ART"},
        {"8.0", "Oreo", "August 21 2017", "26", "ART"},
        {"8.1", "Oreo", "December 5 2017", "27", "ART"},
        {"9", "Pie", "August 6 2018", "28", "ART"},
        {"10", "10", "September 3 2019", "29", "ART"},
        {"11", "11", "September 8 2020", "30", "ART"},
    });
    
    table.SelectAll().Border(LIGHT);
    for (int i = 0; i < 6; i++) {
      table.SelectColumn(i).Border(LIGHT);
      table.SelectRow(i).SeparatorVertical(LIGHT);
      table.SelectRow(0).Border(DOUBLE);
    }
    
    table.SelectColumn(2).DecorateCells(align_right);
    auto content = table.SelectRows(1, -1);
    content.DecorateCellsAlternateRow(color(Color::Blue), 3, 0);
    content.DecorateCellsAlternateRow(color(Color::Cyan), 3, 1);
    content.DecorateCellsAlternateRow(color(Color::White), 3, 2);

    return table.Render();
  });

  // thread used to update counter
  std::thread worker([&]() {
    while (true) {
      // thread sleep for 500ms
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      counter++;
      screen.PostEvent(Event::Custom); // tells ftxui to wake up because of event
    }
  });
  
  screen.Loop(renderer);
}

int main(int argc, char *argv[]) {
  const char *envVariable = std::getenv("TWELVEDATA_API_KEY");
  if (envVariable == nullptr) {
    std::cerr << "api key is null\n";
    exit(-1);
  }
  
  std::string firstLink = "https://api.twelvedata.com/time_series?apikey=";
  std::string apiString = envVariable;

  // only use once because we are only establishing one connection atm
  CURL *curl = curl_easy_init();
  CURLcode result;

  if (curl == NULL) {
    std::cerr << "http request failed.\n";
    return -1;
  }

  // allocate 1 byte and size of 0 so we have something
  // wcb will update accordingly
  struct MemoryStruct chunk;
  chunk.memory = static_cast<char *>(malloc(1));
  chunk.size = {0};

  // making full link with specific symbol/ticker
  std::string endLink = stockSymbol("AAPL");
  std::string fullLink = firstLink + apiString + endLink;

  httpsSetGet(curl, result, chunk, fullLink);

  json data = json::parse(chunk.memory); // parsing the json data directly

  std::string ticker = data["meta"]["symbol"];

  createUI(ticker, data);

  // freeing allocated memory and resetting it to null
  free(chunk.memory);
  chunk.memory = NULL;
  curl_easy_cleanup(curl);
     
  return 0;
}
