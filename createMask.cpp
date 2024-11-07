#include <opencv2/opencv.hpp> 
#include <iostream> 
#include <vector> 
#include <thread> 
#include <mutex> 
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>

//Shared across threads 
std::mutex mtx;
size_t totalMaxPixels = 0;
int processedImages = 0;

//Upper and Lower Thresholds
enum maskThresholds
{
  low_threshold_blue   = 200,
  low_threshold_green  = 200,
  low_threshold_red    = 200,
  high_threshold_blue  = 250,
  high_threshold_green = 250,
  high_threshold_red   = 250
};

/*
   Creates mask based on threshold
   Updates max pixel count
*/
void createROIMask(const std::string& filePath , const std::string& outFilePath)
{

    cv::Mat img = cv::imread(filePath); 
    std::string newOutputFilePath = outFilePath.substr(0, outFilePath.find_last_of('.')) + "_mask.png";
    if (img.empty()) {
        std::cerr << "Image not found!" << std::endl;
        return;
    }
    
    // Pixel in range B:G:R > 200 & < 250 add to mask  
    cv::Mat mask; 
    cv::inRange(img, cv::Scalar(low_threshold_blue, low_threshold_green, low_threshold_red), cv::Scalar(high_threshold_blue, high_threshold_green, high_threshold_red), mask); 
    
    // Update pixel count set to max
    size_t maxPixelCount = cv::countNonZero(mask); 

    { 
       mtx.lock();
       totalMaxPixels += maxPixelCount; 
       mtx.unlock();
    } 

    cv::imwrite(newOutputFilePath, mask);
    
    mtx.lock();
    processedImages++;
    mtx.unlock();
}

int main() {
    
    //Populate Image Dataset
    //Load jpg & png images from directory 
    
    std::vector<std::string> imageDataset; 
    std::vector<std::string> outImages;
    std::string imageDir = "./dataset"; 
    std::string outDir = "./output";
    DIR* dirloc = opendir(imageDir.c_str());
    struct dirent *dp;
    while((dp = readdir(dirloc)) != NULL){

    	std::string temp     = imageDir;
    	std::string temp_out = outDir;
    	temp.append("/");
    	temp.append(dp->d_name);
    	temp_out.append("/");
    	temp_out.append(dp->d_name);
    	size_t found_png = temp.find(".png");
    	size_t found_jpg = temp.find(".jpg");
    	
    	if(found_png != std::string::npos){
    	    imageDataset.push_back(temp);
    	    outImages.push_back(temp_out);
    	}
    	else if(found_jpg != std::string::npos)
    	{
    	    imageDataset.push_back(temp);
    	    outImages.push_back(temp_out);
    	} 

    }
    
    //Check and create output folder if not present
    
    dirloc = opendir(".");
    bool isOutputDir = false; 
    std::string outputDir = "output";
    while((dp = readdir(dirloc)) != NULL){
        if(dp->d_name == outputDir){
           isOutputDir = true;
           std::cout<<"Mask Images Written To Output Folder\n";
        }   
    }
    if(isOutputDir == false)
    {
    	if(mkdir(outputDir.c_str() , 0777) == 0)
    	{
           std::cout<<"Created Output Folder | Mask Images Written To Output Folder\n";   	
    	}
    	else{
    	   std::cout<<"Error Creating Output Folder\n";
    	}
    }
    
    //Process images in seperate threads
    int cnt = 0;
    std::vector<std::thread> threads; 
    for (const auto& path : imageDataset) 
    { 
       const auto& outPath = outImages[cnt];
       threads.emplace_back(createROIMask, path, outPath); 
       cnt++;
    }
    
    for (auto& t : threads) 
    { 
       t.join(); 
    } 
      
    std::cout << "Total images processed : " << processedImages << " | Total max pixels found : " << totalMaxPixels << std::endl;      
    return 0;
}
