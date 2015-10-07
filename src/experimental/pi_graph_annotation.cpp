#include "../core/containers.hpp"
#include "../gui/qttools.hpp"
#include "../gui/utility.hpp"
#include "../gui/singleton.hpp"

#include "pi_graph_annotation_widgets.hpp"
#include "pi_graph_annotation.hpp"

namespace pano {
    namespace experimental {

        int PILayoutAnnotation::getBorder(int c1, int c2) {
            for (int b = 0; b < nborders(); b++) {
                if (border2corners[b].first == c1 && border2corners[b].second == c2)
                    return b;
                if (border2corners[b].first == c2 && border2corners[b].second == c1)
                    return b;
            }
            return -1;
        }

        int PILayoutAnnotation::addBorder(int c1, int c2) {
            if (c1 == c2) {
                std::cout << "the two corners to connect are the same!" << std::endl;
                return -1;
            }
            // connect a border!
            int b = getBorder(c1, c2);
            if (b != -1) {
                std::cout << "border connecting " << c1 << " and " << c2 << " exists already" << std::endl;
                return b;
            }
            border2corners.emplace_back(c1, c2);
            border2connected.push_back(true);
            int borderId = border2corners.size() - 1;
            return borderId;
        }

        int PILayoutAnnotation::splitBorderBy(int b, int c) {
            if (b == -1 || c == -1) {
                return -1;
            }
            int secondCornerOfTheHitBorder = border2corners[b].second;
            border2corners[b].second = c;
            return addBorder(c, secondCornerOfTheHitBorder);
        }

        void PILayoutAnnotation::regenerateFaces() {

            auto oldFace2Plane = std::move(face2plane);
            auto oldFace2Corners = std::move(face2corners);
            auto oldFace2Control = std::move(face2control);

            std::map<std::pair<int, int>, int> corners2border;
            for (int b = 0; b < nborders(); b++) {
                corners2border[border2corners[b]] = b;
            }

            face2corners.clear();
            face2plane.clear();
            coplanarFacePairs.clear();

            std::vector<std::pair<int, int>> border2faces(nborders(), std::make_pair(-1, -1));            

            // rebuild all the faces using corners and borders!
            std::vector<std::vector<int>> corner2orderedAdjacentCorners(ncorners());
            for (int b = 0; b < nborders(); b++) {
                int c1 = border2corners[b].first;
                int c2 = border2corners[b].second;
                assert(c1 != c2);
                assert(!Contains(corner2orderedAdjacentCorners[c2], c1));
                corner2orderedAdjacentCorners[c2].push_back(c1);
                assert(!Contains(corner2orderedAdjacentCorners[c1], c2));
                corner2orderedAdjacentCorners[c1].push_back(c2);
            }
            // order adjacent corners
            for (int c = 0; c < ncorners(); c++) {
                auto & adjs = corner2orderedAdjacentCorners[c];
                auto & dir = corners[c];
                Vec3 x, y;
                std::tie(x, y) = ProposeXYDirectionsFromZDirection(dir);
                std::sort(adjs.begin(), adjs.end(), [&dir, &x, &y, this](int c1, int c2) {
                    auto & dir1 = corners[c1];
                    auto & dir2 = corners[c2];
                    Vec2 dirproj1((dir1 - dir).dot(x), (dir1 - dir).dot(y));
                    Vec2 dirproj2((dir2 - dir).dot(x), (dir2 - dir).dot(y));
                    return SignedAngleBetweenDirections(Vec2(1, 0), dirproj1) < SignedAngleBetweenDirections(Vec2(1, 0), dirproj2);
                });
            }

            // start finding faces            
            while (true) {
                int boundaryBorder = -1;
                bool hasNoLeftFace = true;
                for (int i = 0; i < nborders(); i++) {
                    if (border2faces[i].first == -1) {
                        hasNoLeftFace = true;
                        boundaryBorder = i;
                        break;
                    } else if (border2faces[i].second == -1) {
                        hasNoLeftFace = false;
                        boundaryBorder = i;
                        break;
                    }
                }

                if (boundaryBorder == -1) { // all boundaries are connected
                    break;
                }

                int fromC, toC;
                std::tie(fromC, toC) = border2corners[boundaryBorder];
                if (!hasNoLeftFace) {
                    std::swap(fromC, toC);
                }

                face2corners.push_back(std::vector<int>());
                int faceId = face2corners.size() - 1;

                while (true) {
                    int & leftFace = Contains(corners2border, std::make_pair(fromC, toC)) ?
                        border2faces[corners2border.at(std::make_pair(fromC, toC))].first :
                        border2faces[corners2border.at(std::make_pair(toC, fromC))].second;
                    if (leftFace != -1) {
                        break;
                    }

                    leftFace = faceId;
                    face2corners[faceId].push_back(toC);

                    // move to next 
                    auto & orderedAdjCs = corner2orderedAdjacentCorners[toC];
                    int fromPositionInAdjCs = -1;
                    for (int i = 0; i < orderedAdjCs.size(); i++) {
                        if (orderedAdjCs[i] == fromC) {
                            fromPositionInAdjCs = i;
                            break;
                        }
                    }
                    assert(fromPositionInAdjCs != -1);

                    int nextC = orderedAdjCs[(fromPositionInAdjCs + 1) % orderedAdjCs.size()];
                    assert(nextC != fromC);
                    fromC = toC;
                    toC = nextC;
                }
            }

        
            face2plane.resize(face2corners.size());
            face2control.resize(face2corners.size(), SegControl{ -1, -1, true });
        }

        int PILayoutAnnotation::setCoplanar(int f1, int f2) {
            if (f1 == f2 || f1 == -1 || f2 == -1)
                return -1;
            for (int i = 0; i < coplanarFacePairs.size(); i++) {
                auto & fp = coplanarFacePairs[i];
                if (fp.first == f1 && fp.second == f2 || fp.first == f2 && fp.second == f1) {
                    return i;
                }
            }
            coplanarFacePairs.emplace_back(f1, f2);
            return coplanarFacePairs.size() - 1;
        }

     

        std::string LayoutAnnotationFilePath(const std::string & imagePath) {
            QFileInfo finfo(QString::fromStdString(imagePath));
            if (!finfo.exists())
                return "";
            auto annoFileName = finfo.absoluteFilePath() + ".layoutanno.cereal";
            return annoFileName.toStdString();
        }

        std::string TempLayoutAnnotationFilePath(const std::string & imagePath) {
            QFileInfo finfo(QString::fromStdString(imagePath));
            if (!finfo.exists())
                return "";
            auto annoFileName = finfo.absoluteFilePath() + ".templayoutanno.cereal";
            return annoFileName.toStdString();
        }


        PILayoutAnnotation LoadOrInitializeNewLayoutAnnotation(const std::string & imagePath) {
            assert(QFileInfo(QString::fromStdString(imagePath)).exists());

            auto annoPath = LayoutAnnotationFilePath(imagePath);
            QFileInfo annofinfo(QString::fromStdString(annoPath));

            PILayoutAnnotation anno;

            // if exist, load it
            if (!annofinfo.exists() || !LoadFromDisk(annofinfo.absoluteFilePath().toStdString(), anno)) {

                // initialize new annotation
                anno.originalImage = cv::imread(imagePath);

                // rectify the image
                std::cout << "rectifying image" << std::endl;
                anno.rectifiedImage = anno.originalImage.clone();
                gui::MakePanoramaByHand(anno.rectifiedImage, &anno.extendedOnTop, &anno.extendedOnBottom);

                // create view
                std::cout << "creating views" << std::endl;
                Image image = anno.rectifiedImage.clone();
                ResizeToHeight(image, 700);
                anno.view = CreatePanoramicView(image);

                // collect lines
                std::cout << "collecting lines" << std::endl;
                auto cams = CreateCubicFacedCameras(anno.view.camera, image.rows, image.rows, image.rows * 0.4);
                std::vector<Line3> rawLine3s;
                for (int i = 0; i < cams.size(); i++) {
                    auto pim = anno.view.sampled(cams[i]).image;
                    LineSegmentExtractor lineExtractor;
                    lineExtractor.params().algorithm = LineSegmentExtractor::LSD;
                    auto ls = lineExtractor(pim, 3, 300); // use pyramid
                    for (auto & l : ls) {
                        rawLine3s.emplace_back(normalize(cams[i].toSpace(l.first)),
                            normalize(cams[i].toSpace(l.second)));
                    }
                }
                rawLine3s = MergeLines(rawLine3s, DegreesToRadians(1));

                // estimate vp and initially classify lines
                std::cout << "estimating vps and clasifying lines" << std::endl;
                auto line3s = ClassifyEachAs(rawLine3s, -1);
                auto vps = EstimateVanishingPointsAndClassifyLines(line3s);
                OrderVanishingPoints(vps);
                int vertVPId = NearestDirectionId(vps, Vec3(0, 0, 1));

                anno.vps = std::move(vps);
                anno.vertVPId = vertVPId;
            }

            return anno;
        }

        void EditLayoutAnnotation(const std::string & imagePath, PILayoutAnnotation & anno) {
            gui::Singleton::InitGui();
            PILayoutAnnotationWidget w;
            QString impath = QString::fromStdString(imagePath);
            w.setCurAnnotation(&anno, &impath);
            w.resize(900, 900);
            w.show();
            gui::Singleton::ContinueGui();
        }

        void SaveLayoutAnnotation(const std::string & imagePath, const PILayoutAnnotation & anno) {
            auto annoPath = LayoutAnnotationFilePath(imagePath);
            QFileInfo annofinfo(QString::fromStdString(annoPath));
            SaveToDisk(annofinfo.absoluteFilePath().toStdString(), anno);
        }

        void SaveTempLayoutAnnotation(const std::string & imagePath, const PILayoutAnnotation & anno) {
            auto annoPath = TempLayoutAnnotationFilePath(imagePath);
            QFileInfo annofinfo(QString::fromStdString(annoPath));
            SaveToDisk(annofinfo.absoluteFilePath().toStdString(), anno);
        }

    }
}