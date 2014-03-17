#ifndef PANORAMIX_CORE_PANORAMA_HPP
#define PANORAMIX_CORE_PANORAMA_HPP

#include <Eigen/Core>
#include <opencv2/opencv.hpp>

namespace panoramix {
	namespace core {
        
        using cv::Mat;
        using Eigen::Vector2d;
        using Eigen::Vector3d;
        
        
        class View {
        public:
            
        private:

        };
        

        class Image {
        public:

        private:
            View * _view;
            cv::Mat _im;
        };
        
 
	}
}
 
#endif