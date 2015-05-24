#include <boost/filesystem.hpp>

#include "../../src/core/basic_types.hpp"
#include "../../src/core/single_view.hpp"
#include "../../src/experimental/rl_graph.hpp"
#include "../../src/experimental/tools.hpp"
#include "../../src/experimental/solve_connection.hpp"
#include "../../src/misc/matlab_engine.hpp"
#include "../../src/misc/matlab_api.hpp"
#include "../../src/gui/scene.hpp"
#include "../../src/gui/canvas.hpp"
#include "../../src/gui/utility.hpp"

#include "routines.hpp"


namespace panolyz {

    namespace PanoContext {

        static const std::string root = "F:\\DataSets\\panoContext_data\\bedroom";

        void Run(){

            using namespace panoramix;
            using namespace core;
            using namespace experimental;

            misc::MAT annotations(root + "\\ANNO_ALL.mat", misc::MAT::OpeningMode::Read);
            auto anno = annotations.var("ANNO_ALL");
            int annoLength = anno.length();

            std::cout << "INPUT ID [0~441 except 12]: " << std::endl;
            int id = 0;
            std::cin >> id;
            std::cout << "ID = " << id << std::endl;
            std::string name = anno.field("name", id);
            std::cout << "NAME = " << name << std::endl;

            // image
            std::string path = root + "\\" + name + "\\" + name + ".jpg";
            auto image = gui::ImageRead(path);

            /// gt
            // vps
            DenseMatd vpsData = anno.field("vp", id);
            assert(vpsData.rows == 6 && vpsData.cols == 3);
            std::vector<Vec3> vps(3);
            for (int i = 0; i < 3; i++){
                for (int j = 0; j < 3; j++){
                    vps[i][j] = vpsData(i, j);
                }
            }
            // objects
            auto objects3DData = anno.field("ANNO3D", id).field("objects3D");
            int nobjects = objects3DData.length();
            struct Object {
                std::vector<Point3> points;
                int type;
                std::string name;
            };
            std::vector<Object> objects(nobjects);
            for (int i = 0; i < nobjects; i++){
                DenseMatd points = objects3DData.field("out_points_3", i);
                objects[i].points.resize(points.rows);
                for (int j = 0; j < points.rows; j++){
                    for (int k = 0; k < 3; k++){
                        objects[i].points[j][k] = points(j, k);
                    }
                }
                objects[i].type = objects3DData.field("type", i);
                objects[i].name = objects3DData.field("name", i);
            }



            MakePanorama(image);
            ResizeToHeight(image, 700);


            View<PanoramicCamera, Image3ub> view;
            std::vector<PerspectiveCamera> cams;
            std::vector<std::vector<Classified<Line2>>> lines;
            int vertVPId;
            Imagei segmentedImage;
            int nsegments;

#define REFRESH 1

            if (REFRESH){
                view = CreatePanoramicView(image);

                // collect lines in each view
                cams = CreateCubicFacedCameras(view.camera, image.rows, image.rows, image.rows * 0.4);
                lines.resize(cams.size());
                for (int i = 0; i < cams.size(); i++){
                    auto pim = view.sampled(cams[i]).image;
                    LineSegmentExtractor lineExtractor;
                    lineExtractor.params().algorithm = LineSegmentExtractor::LSD;
                    auto ls = lineExtractor(pim, 2, 300); // use pyramid
                    lines[i].reserve(ls.size());
                    for (auto & l : ls){
                        lines[i].push_back(ClassifyAs(l, -1));
                    }
                }

                // estimate vp
                vps = EstimateVanishingPointsAndClassifyLines(cams, lines);
                if (0){
                    auto ctable = gui::CreateRandomColorTableWithSize(vps.size());
                    for (int i = 0; i < cams.size(); i++){
                        auto pim = view.sampled(cams[i]).image;
                        gui::AsCanvas(pim).thickness(3).colorTable(ctable).add(lines[i]).show();
                    }
                }
                vertVPId = NearestDirectionId(vps, Vec3(0, 0, 1));

                // extract lines from segmentated region boundaries and classify them using estimated vps
                // make 3d lines
                std::vector<Line3> line3ds;
                for (int i = 0; i < cams.size(); i++){
                    for (auto & l : lines[i]){
                        line3ds.emplace_back(normalize(cams[i].toSpace(l.component.first)),
                            normalize(cams[i].toSpace(l.component.second)));
                    }
                }

                SegmentationExtractor segmenter;
                segmenter.params().algorithm = SegmentationExtractor::GraphCut;
                segmenter.params().sigma = 10.0;
                segmenter.params().c = 1.0;
                segmenter.params().superpixelSizeSuggestion = 2000;
                std::tie(segmentedImage, nsegments) = segmenter(view.image, line3ds, view.camera, M_PI / 36.0);

                if (0){
                    auto ctable = gui::CreateRandomColorTableWithSize(nsegments);
                    gui::AsCanvas(ctable(segmentedImage)).show();
                    gui::AsCanvas(ctable(segmentedImage)).add(view.image).show();
                }

                Save(path, "pre", view, cams, lines, vps, segmentedImage, nsegments, vertVPId);
            }
            else{
                Load(path, "pre", view, cams, lines, vps, segmentedImage, nsegments, vertVPId);
            }




            std::vector<PerspectiveCamera> hcams;
            if (REFRESH){
                hcams = CreatePanoContextCameras(view.camera, 500, 400, 300);
                Save(path, "hcams", hcams);
            }
            else{
                Load(path, "hcams", hcams);
            }


            // extract gcs
            std::vector<Weighted<View<PerspectiveCamera, Image5d>>> gcs;
            if (REFRESH){               
                gcs.resize(hcams.size());
                misc::MatlabEngine matlab;
                for (int i = 0; i < hcams.size(); i++){
                    auto pim = view.sampled(hcams[i]);
                    auto pgc = ComputeGeometricContext(pim.image, false, true);
                    gcs[i].component.camera = hcams[i];
                    gcs[i].component.image = pgc;
                    gcs[i].score = sin(AngleBetweenUndirectedVectors(hcams[i].forward(), view.camera.up()));
                }
                Save(path, "gcs", gcs);
            }
            else{
                Load(path, "gcs", gcs);
            }

            Image5d gc;
            if (REFRESH){
                gc = Combine(view.camera, gcs).image;
                Save(path, "gc", gc);
            }
            else{
                Load(path, "gc", gc);
            }
            if (1){
                gui::AsCanvas(gc).show();
            }

            // extract oms
            std::vector<View<PerspectiveCamera, Image3f>> oms;
            if (1){
                oms.resize(hcams.size());
                LineSegmentExtractor lineSegExtractor;
                lineSegExtractor.params().algorithm = LineSegmentExtractor::LSD;
                for (int i = 0; i < hcams.size(); i++){
                    auto pim = view.sampled(hcams[i]);
                    auto lines = lineSegExtractor(pim.image);
                    std::vector<HPoint2> hvps(vps.size());
                    for (int k = 0; k < vps.size(); k++){
                        hvps[k] = pim.camera.toScreenInHPoint(vps[k]);
                    }
                    auto clines = ClassifyEachAs(lines, -1);
                    ClassifyLines(clines, hvps, DegreesToRadians(8), 0.8);
                    auto omi = ComputeOrientationMaps(clines, hvps, pim.image.size());
                    Image3f om(omi.size(), Vec3f());
                    for (auto it = om.begin(); it != om.end(); ++it){
                        int id = omi(it.pos());
                        if (id < 0)
                            continue;
                        (*it)(id) = 1.0f;
                    }
                    oms[i].camera = hcams[i];
                    oms[i].image = om;
                }
                Save(path, "oms", oms);
            }
            else{
                Load(path, "oms", oms);
            }

            Image3f om;
            if (REFRESH){
                om = Combine(view.camera, oms).image;
                for (auto & omp : om){
                    omp /= std::max(1e-5, core::norm(omp));
                }
                Save(path, "om", om);
            }
            else{
                Load(path, "om", om);
            }
            if (1){
                gui::AsCanvas(om).show();
            }



            // collect region info
            std::vector<std::vector<std::vector<Vec3>>> regionContours;
            std::vector<Vec3> regionCenterDirs;
            auto regionIds = ComputeSpatialRegionProperties(segmentedImage, view.camera, &regionContours, &regionCenterDirs);

            std::vector<Vec5> gcMeans(regionIds.size());
            for (int i = 0; i < regionIds.size(); i++){
                auto pview = PerfectRegionMaskView(regionContours[i], regionCenterDirs[i], 300);
                gcMeans[i] = Mean(MakeCameraSampler(pview.camera, view.camera)(gc), pview.image);
            }



            // object meshes
            std::vector<Mesh<Point3>> objMeshes;
            for (int i = 0; i < objects.size(); i++){
                if (objects[i].name == "room")
                    continue;

            }


            ProjectiveComponentArray components;
            components.reserve(nsegments + lines.size());


        }
    }
}