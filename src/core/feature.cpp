#include "feature.hpp"

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <cstdint>
#include <chrono>

namespace panoramix {
	namespace core {
 
		void ExtractLines( const Mat& im,
                          std::list<LineData<double, 2>> & lines,
                          int minlen, int xborderw, int yborderw, int numdir) {
			using namespace std::chrono;
			auto start = high_resolution_clock::now();
            
			cv::Mat gim;
			cv::cvtColor(im, gim, CV_BGR2GRAY);
			int h = gim.rows;
			int w = gim.cols;
            
			cv::Mat dx, dy;
			cv::Mat ggim;
			cv::GaussianBlur(gim, ggim, cv::Size(7,7), 1.5);
			cv::Sobel(ggim, dx, CV_64F, 1, 0);
			cv::Sobel(ggim, dy, CV_64F, 0, 1);
            
			cv::Mat imcanny;
			cv::Canny(gim, imcanny, 5, 20);
            
			std::vector<std::unordered_set<int>> pixelidsset(numdir);
            
			for(int x = 0; x < imcanny.cols; x++){
				for(int y = 0; y < imcanny.rows; y++){
					if(imcanny.at<uchar>(y, x) > 0){
						double a = atan(dy.at<double>(y, x) / dx.at<double>(y, x));
						if(a != a){ // NaN
							continue;
						}
						// compute bin id
						int binid = int((a / M_PI + 0.5) * numdir);
						if(binid == -1) binid = 0;
						if(binid == numdir) binid = numdir - 1;
                        
						int pixelid = y + imcanny.rows * x;
                        
						pixelidsset[(binid + numdir -1) % numdir].insert(pixelid);
						pixelidsset[binid].insert(pixelid);
						pixelidsset[(binid + 1) % numdir].insert(pixelid);
					}
				}
			}
            
			std::vector<int> xs, ys, ids;
			xs.reserve(512);
			ys.reserve(512);
			ids.reserve(512);
            
			for(int binid = 0; binid < numdir; binid++){
				// search in bins
				auto pixelidnotsearchedyet = pixelidsset[binid];
                
				while(true){
					if(pixelidnotsearchedyet.empty())
						break;
					int rootid = *pixelidnotsearchedyet.begin();
                    
					// BFS
					xs.clear(); ys.clear(); ids.clear();
					int x = rootid / imcanny.rows;
					int y = rootid - imcanny.rows * x;
					xs.push_back(x);
					ys.push_back(y);
					ids.push_back(rootid);
                    
					// used for search only
					pixelidnotsearchedyet.erase(rootid);
					int head = 0;
                    
					static const int xdirs[] = {1, 1, 0, -1, -1, -1, 0, 1};
					static const int ydirs[] = {0, 1, 1, 1, 0, -1, -1, -1};
					while(true){
						if(head == xs.size())
							break;
						x = xs[head];
						y = ys[head];
						for(int k = 0; k < 8; k++){
							int nx = x + xdirs[k];
							int ny = y + ydirs[k];
							int npixelid = ny + imcanny.rows * nx;
							if(pixelidnotsearchedyet.find(npixelid) != pixelidnotsearchedyet.end()){
								xs.push_back(nx);
								ys.push_back(ny);
								ids.push_back(npixelid);
								pixelidnotsearchedyet.erase(npixelid);
							}
						}
						head++;
					}
                    
					int edgesize = xs.size();
					if(edgesize < minlen)
						continue;
                    
					cv::Mat xsmat(xs), ysmat(ys);
					double meanx = cv::mean(xsmat).val[0], meany = cv::mean(ysmat).val[0];
					cv::Mat zmx = xsmat - meanx, zmy = ysmat - meany;
                    
					cv::Mat v, lambda;
					cv::Mat D(2, 2, CV_64FC1);
					D.at<double>(0, 0) = cv::sum(zmx.mul(zmx)).val[0];
					D.at<double>(0, 1) = D.at<double>(1, 0) = cv::sum(zmx.mul(zmy)).val[0];
					D.at<double>(1, 1) = cv::sum(zmy.mul(zmy)).val[0];
					cv::eigen(D, true, lambda, v);
                    
					double theta = atan2(v.at<double>(0, 1), v.at<double>(0, 0));
					double confidence = std::numeric_limits<double>::max();
					if(lambda.at<double>(1) > 0){
						confidence = lambda.at<double>(0) / lambda.at<double>(1);
					}
                    
					// build line
					if(confidence >= 400){
						for(int pid : ids){
							pixelidsset[binid].erase(pid);
							pixelidsset[(binid - 1 + numdir) % numdir].erase(pid);
							pixelidsset[(binid + 1) % numdir].erase(pid);
						}
                        
						auto xends = std::minmax_element(xs.begin(), xs.end());
						auto yends = std::minmax_element(ys.begin(), ys.end());
						double minx = *xends.first, maxx = *xends.second;
						double miny = *yends.first, maxy = *yends.second;
                        
						if(maxx <= xborderw || minx >= w - xborderw || maxy <= yborderw || miny >= h - yborderw)
							continue;
                        
						double len = sqrt((maxx - minx)*(maxx - minx) + (maxy - miny)*(maxy - miny));
						double x1 = meanx - cos(theta) * len/2;
						double x2 = meanx + cos(theta) * len/2;
						double y1 = meany - sin(theta) * len/2;
						double y2 = meany + sin(theta) * len/2;
                        
						//linept1s.push_back(cv::Vec2d(x1, y1));
						//linept2s.push_back(cv::Vec2d(x2, y2));
                        lines.push_back({cv::Vec2d(x1, y1), cv::Vec2d(x2, y2)});
					}
				}			
			}
            
			auto time = duration_cast<duration<double>>(high_resolution_clock::now() - start);
			std::cout << "extractlines time: " << time.count() << std::endl;
		}
 
	}
}

