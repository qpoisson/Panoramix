#pragma once

#include "../core/basic_types.hpp"
#include "../core/utility.hpp"

#include "pi_graph.hpp"

namespace pano {
    namespace experimental {
        
        using namespace pano::core;

        struct PILayoutAnnotation {
            Image originalImage;
            Image rectifiedImage;
            bool extendedOnTop;
            bool extendedOnBottom;

            PanoramicView view;
            std::vector<Vec3> vps;
            int vertVPId;
                        
            std::vector<Vec3> corners;            
            std::vector<std::pair<int, int>> border2corners; // from, to
            std::vector<bool> border2connected;

            std::vector<std::vector<int>> face2corners;
            std::vector<SegControl> face2control;
            std::vector<Plane3> face2plane;
            std::vector<std::pair<int, int>> coplanarFacePairs;
            std::vector<Polygon3> clutters;

            PILayoutAnnotation() : vertVPId(-1) {}
            
            int ncorners() const { return corners.size(); }
            int nborders() const { return border2corners.size(); }
            int nfaces() const { return face2corners.size(); }

            int getBorder(int c1, int c2);
            int addBorder(int c1, int c2);
            int splitBorderBy(int b, int c);

            void regenerateFaces();
            int setCoplanar(int f1, int f2);

            template <class Archiver>
            inline void serialize(Archiver & ar, std::int32_t version) {
                if (version == 0) {
                    ar(originalImage, rectifiedImage, extendedOnTop, extendedOnBottom);
                    ar(view, vps, vertVPId);
                    ar(corners, border2corners, border2connected);
                    ar(face2corners, face2control, face2plane, coplanarFacePairs, clutters);
                } else {
                    NOT_IMPLEMENTED_YET();
                }
            }
        };

        std::string LayoutAnnotationFilePath(const std::string & imagePath);

        PILayoutAnnotation LoadOrInitializeNewLayoutAnnotation(const std::string & imagePath);

        void EditLayoutAnnotation(const std::string & imagePath, PILayoutAnnotation & anno);

        void SaveLayoutAnnotation(const std::string & imagePath, const PILayoutAnnotation & anno);

    }
}


//CEREAL_CLASS_VERSION(pano::experimental::PIAnnotation, 0);

CEREAL_CLASS_VERSION(pano::experimental::PILayoutAnnotation, 0);