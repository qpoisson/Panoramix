#include <QtWidgets>

#include "../../src/core/factor_graph.hpp"
#include "../../src/core/parallel.hpp"
#include "../../src/misc/cache.hpp"
#include "../../src/misc/clock.hpp"

#include "../../src/gui/canvas.hpp"
#include "../../src/gui/qttools.hpp"
#include "../../src/gui/scene.hpp"
#include "../../src/gui/singleton.hpp"
#include "../../src/gui/utility.hpp"

#include "../../src/experimental/line_drawing.hpp"
#include "../../src/experimental/pi_graph_annotation.hpp"
#include "../../src/experimental/pi_graph_cg.hpp"
#include "../../src/experimental/pi_graph_control.hpp"
#include "../../src/experimental/pi_graph_occlusion.hpp"
#include "../../src/experimental/pi_graph_optimize.hpp"
#include "../../src/experimental/pi_graph_vis.hpp"

using namespace pano;
using namespace pano::core;
using namespace pano::experimental;

#define DISABLED_main MACRO_CONCAT(main_, __COUNTER__)

// VHFSelectedFunT: (VertHandle/HalfHandle/FaceHandle) -> bool
// VertPositionFunT: (VertHandle)->Point3
// HalfColorerFunT: (HalfHandle)->gui::Color
// FaceColorerFunT: (FaceHandle)->gui::Color
template <class VertDataT, class HalfDataT, class FaceDataT,
          class VHFSelectedFunT, class VertPositionFunT, class HalfColorerFunT,
          class FaceColorerFunT>
void AddToScene(gui::SceneBuilder &sb,
                const Mesh<VertDataT, HalfDataT, FaceDataT> &m,
                VHFSelectedFunT selected, VertPositionFunT vertPosFun,
                HalfColorerFunT colorHalf, FaceColorerFunT colorFace) {
  sb.installingOptions().lineWidth = 1.0;
  sb.installingOptions().defaultShaderSource =
      gui::OpenGLShaderSourceDescriptor::XLines;
  HandledTable<HalfHandle, int> added(m.internalHalfEdges().size(), false);
  for (auto &h : m.halfedges()) {
    if (!selected(h.topo.hd) || !selected(h.topo.from()) ||
        !selected(h.topo.to())) {
      continue;
    }
    Line3 line(vertPosFun(m.data(h.topo.from())),
               vertPosFun(m.data(h.topo.to())));
    auto hh = h.topo.hd;
    auto fh = h.topo.face;
    auto oppohh = h.topo.opposite;
    auto oppofh = oppohh.valid() ? m.topo(oppohh).face : FaceHandle();
    bool hasFace = fh.valid();
    bool hasOppo = oppohh.valid();
    gui::Color color = colorHalf(hh);
    if (color.isTransparent()) {
      continue;
    }
    if (!hasFace && hasOppo) {
      color = gui::Red;
    } else if (hasFace && !hasOppo) {
      color = gui::Blue;
    } else if (!hasFace && !hasOppo) {
      color = gui::Yellow;
    }
    if (added[oppohh]) {
      continue;
    }
    sb.add(gui::ColorAs(line, color), [hh, fh, oppohh, oppofh](auto &...) {
      std::cout << "halfedge id: " << hh.id
                << ", opposite halfedge id: " << oppohh.id
                << ", face id: " << fh.id << ", opposite face id: " << oppofh.id
                << '\n';
    });
    added[hh] = true;
  }
  sb.installingOptions().defaultShaderSource =
      gui::OpenGLShaderSourceDescriptor::XTriangles;
  for (auto &f : m.faces()) {
    if (!selected(f.topo.hd)) {
      continue;
    }
    Polygon3 poly;
    for (auto h : f.topo.halfedges) {
      auto v = m.topo(h).to();
      poly.corners.push_back(vertPosFun(m.data(v)));
    }
    assert(poly.corners.size() > 2);
    poly.normal = (poly.corners[0] - poly.corners[1])
                      .cross(poly.corners[0] - poly.corners[2]);
    auto fh = f.topo.hd;
    gui::Color color = colorFace(f.topo.hd);
    if (color.isTransparent()) {
      continue;
    }
    sb.add(gui::ColorAs(poly, color),
           [fh](auto &...) { std::cout << "face id: " << fh.id << '\n'; });
  }
}

inline std::vector<Point2> PossibleKeyVanishingPoints(const Chain2 &chain) {
  assert(chain.size() > 2);
  if (chain.size() == 3) {
    return {};
  }
  if (chain.size() == 4) {
    return {Intersection(chain.edge(0).ray(), chain.edge(2).ray()),
            Intersection(chain.edge(1).ray(), chain.edge(3).ray())};
  }
  if (chain.size() % 2 == 0) {
    std::vector<Point2> vpPositions;
    vpPositions.reserve(chain.size() * chain.size() / 4);
    for (int i = 0; i < chain.size() / 2; i++) {
      for (int j = i + 1; j < i + chain.size() / 2; j++) {
        vpPositions.push_back(
            Intersection(Line2(chain.at(i), chain.at(j)).ray(),
                         Line2(chain.at(i + chain.size() / 2),
                               chain.at(j + chain.size() / 2))
                             .ray()));
      }
    }
    return vpPositions;
  } else {
    std::vector<Point2> vpPositions;
    for (int i = 0; i < chain.size(); i++) {
      for (int j = i + 1; j < chain.size(); j++) {
        vpPositions.push_back(
            Intersection(chain.edge(i).ray(), chain.edge(j).ray()));
      }
    }
    return vpPositions;
  }
}

int main(int argc, char **argv) {
  gui::Singleton::InitGui(argc, argv);
  misc::SetCachePath("D:\\Panoramix\\LineDrawing\\");

  std::string name = "hex";
  std::string camName = "cam1";
  bool resetCam = false;

  std::string objFile = "H:\\GitHub\\Panoramix\\data\\linedrawing\\" + name +
                        "\\" + name + ".obj";
  std::string camFile = "H:\\GitHub\\Panoramix\\data\\linedrawing\\" + name +
                        "\\" + name + ".obj." + camName + ".cereal";


  //// [Load Mesh]
  auto mesh = LoadFromObjFile(objFile);
  auto meshProxy = MakeMeshProxy(mesh);


  //// [Decompose]
  auto cutFacePairs = DecomposeAll(
      meshProxy, [](HalfHandle hh1, HalfHandle hh2) -> bool { return false; });
  std::unordered_map<FaceHandle, FaceHandle> cutFace2Another;
  for (auto &cutFacePair : cutFacePairs) {
    cutFace2Another[cutFacePair.first] = cutFacePair.second;
    cutFace2Another[cutFacePair.second] = cutFacePair.first;
  }
  auto subMeshes = ExtractSubMeshes(meshProxy,
                                    [](auto hhbegin, auto hhend) -> bool {
                                      return std::distance(hhbegin, hhend) <= 1;
                                    },
                                    10);
  Println("found ", subMeshes.size(), " subMeshes");


  //// [Load Camera]
  PerspectiveCamera cam;
  if (!LoadFromDisk(camFile, cam) || resetCam) {
    auto sphere = BoundingBoxOfContainer(mesh.vertices()).outerSphere();
    PerspectiveCamera projCam(500, 500, Point2(250, 250), 200,
                              sphere.center + Vec3(1, 2, 3) * sphere.radius * 2,
                              sphere.center);

    const gui::ColorTable ctable =
        gui::CreateRandomColorTableWithSize(subMeshes.size());

    HandledTable<HalfHandle, int> hh2subMeshId(
        meshProxy.internalHalfEdges().size(), -1);
    for (auto &h : meshProxy.halfedges()) {
      int &id = hh2subMeshId[h.topo.hd];
      for (int i = 0; i < subMeshes.size(); i++) {
        if (subMeshes[i].contains(h.topo.hd)) {
          id = i;
          break;
        }
      }
    }
    HandledTable<FaceHandle, int> fh2subMeshId(meshProxy.internalFaces().size(),
                                               -1);
    for (auto &f : meshProxy.faces()) {
      int &id = fh2subMeshId[f.topo.hd];
      for (int i = 0; i < subMeshes.size(); i++) {
        if (subMeshes[i].contains(f.topo.hd)) {
          id = i;
          break;
        }
      }
    }

    // show each subMesh
    if (true) {
      for (int i = 0; i < subMeshes.size(); i++) {
        Println("subMesh - ", i);
        gui::SceneBuilder sb;
        sb.installingOptions().defaultShaderSource =
            gui::OpenGLShaderSourceDescriptor::XTriangles;
        sb.installingOptions().discretizeOptions.color(gui::Black);
        sb.installingOptions().lineWidth = 0.03;
        AddToScene(
            sb, meshProxy,
            [&subMeshes, i](auto h) { return subMeshes[i].contains(h); },
            [&mesh](VertHandle vh) { return mesh.data(vh); },
            [&hh2subMeshId, &ctable](HalfHandle hh) { return gui::Black; },
            [&fh2subMeshId, &ctable](FaceHandle fh) {
              return ctable[fh2subMeshId[fh]];
            });
        sb.show(true, false, gui::RenderOptions()
                                 .camera(projCam)
                                 .backgroundColor(gui::White)
                                 .renderMode(gui::All)
                                 .bwTexColor(0.0)
                                 .bwColor(1.0)
                                 .fixUpDirectionInCameraMove(false)
                                 .cullBackFace(false)
                                 .cullFrontFace(false));
      }
    }

    { // show together
      gui::SceneBuilder sb;
      sb.installingOptions().defaultShaderSource =
          gui::OpenGLShaderSourceDescriptor::XTriangles;
      sb.installingOptions().discretizeOptions.color(gui::Black);
      sb.installingOptions().lineWidth = 0.03;
      AddToScene(sb, meshProxy, [](auto) { return true; },
                 [&mesh](VertHandle vh) { return mesh.data(vh); },
                 [&hh2subMeshId, &ctable](HalfHandle hh) { return gui::Black; },
                 [&fh2subMeshId, &ctable](FaceHandle fh) {
                   return ctable[fh2subMeshId[fh]];
                 });
      cam = sb.show(true, false, gui::RenderOptions()
                                     .camera(projCam)
                                     .backgroundColor(gui::White)
                                     .renderMode(gui::All)
                                     .bwTexColor(0.0)
                                     .bwColor(1.0)
                                     .fixUpDirectionInCameraMove(false)
                                     .cullBackFace(false)
                                     .cullFrontFace(false))
                .camera();
    }
    SaveToDisk(camFile, cam);
  }

  //// [Make 2D Mesh]
  // convert to 2d
  auto mesh2d = Transform(
      mesh, [&cam](const Point3 &p) -> Point2 { return cam.toScreen(p); });

  // add offset noise
  Vec2 offsetNoise = Vec2(20, -20);
  for (auto &v : mesh2d.vertices()) {
    v.data += offsetNoise;
  }

  if(true){
    Image3ub im(cam.screenSize(), Vec3ub(255, 255, 255));
    auto canvas = gui::MakeCanvas(im);
    canvas.color(gui::Black);
    canvas.thickness(2);
    for (auto &h : mesh2d.halfedges()) {
      auto &p1 = mesh2d.data(h.topo.from());
      auto &p2 = mesh2d.data(h.topo.to());
      canvas.add(Line2(p1, p2));
    }
    canvas.show(0, "mesh2d");
  }


  //// [Estimate PP & Focal Candidates from 2D Mesh]
  auto point2dAt = [&mesh2d, &meshProxy](VertHandle vhInProxy) -> Point2 {
    return mesh2d.data(meshProxy.data(vhInProxy));
  };
  auto line2dAt = [&mesh2d, &meshProxy,
                   point2dAt](HalfHandle hhInProxy) -> Line2 {
    return Line2(point2dAt(meshProxy.topo(hhInProxy).from()),
                 point2dAt(meshProxy.topo(hhInProxy).to()));
  };
  Box2 box = BoundingBoxOfContainer(mesh2d.vertices());
  double scale = box.outerSphere().radius;

  struct PPFocalCandidate {
    Point2 pp;
    double focal;
  };

  // collect pp focal candidates
  std::vector<PPFocalCandidate> ppFocalCandidates;
  ppFocalCandidates.reserve(subMeshes.size() * 3);

  for (int subMeshId = 0; subMeshId < subMeshes.size(); subMeshId++) {
    // collect edge intersections in each face
    std::vector<Point2> interps;
    for (auto fh : subMeshes[subMeshId].fhs) {
      auto &hhs = meshProxy.topo(fh).halfedges;
      Chain2 corners;
      for (auto hh : hhs) {
        corners.append(point2dAt(meshProxy.topo(hh).to()));
      }
      auto keyVPs = PossibleKeyVanishingPoints(corners);
      interps.insert(interps.end(), keyVPs.begin(), keyVPs.end());
    }

    for (int i = 0; i < interps.size(); i++) {
      const Point2 &p1 = interps[i];
      for (int j = i + 1; j < interps.size(); j++) {
        const Point2 &p2 = interps[j];
        for (int k = j + 1; k < interps.size(); k++) {
          const Point2 &p3 = interps[k];
          // compute pp and focal
          Point2 pp;
          double focal = 0.0;
          std::tie(pp, focal) = ComputePrinciplePointAndFocalLength(p1, p2, p3);
          if (HasValue(pp, IsInfOrNaN<double>) || IsInfOrNaN(focal)) {
            continue;
          }
          if (!IsBetween(focal, scale / 5.0, scale * 5.0) ||
              Distance(pp, box.center()) > scale * 2.0) {
            continue;
          }
          ppFocalCandidates.push_back(PPFocalCandidate{pp, focal});
        }
      }
    }
  }

  std::sort(ppFocalCandidates.begin(), ppFocalCandidates.end(),
            [](auto &a, auto &b) { return a.focal < b.focal; });
  // naive clustering
  std::vector<std::pair<std::set<int>, PPFocalCandidate>> ppFocalGroups;
  {
    std::vector<int> ppFocalId2group(ppFocalCandidates.size(), -1);
    int ngroups = 0;
    RTreeMap<Vec3, int> ppFocalIdTree;
    for (int i = 0; i < ppFocalCandidates.size(); i++) {
      Vec3 coordinate =
          cat(ppFocalCandidates[i].pp, ppFocalCandidates[i].focal);
      const double thres = scale / 50.0;
      // find the nearest ppFocal sample point
      int nearestPPFocalCandId = -1;
      double minDist = thres;
      ppFocalIdTree.search(BoundingBox(coordinate).expand(thres * 2),
                           [&nearestPPFocalCandId, &minDist,
                            &coordinate](const std::pair<Vec3, int> &cand) {
                             double dist = Distance(cand.first, coordinate);
                             if (dist < minDist) {
                               minDist = dist;
                               nearestPPFocalCandId = cand.second;
                             }
                             return true;
                           });
      if (nearestPPFocalCandId != -1) { // if found, assign to the same group
        ppFocalId2group[i] = ppFocalId2group[nearestPPFocalCandId];
      } else { // otherwise, create a new group
        ppFocalId2group[i] = ngroups++;
      }
      ppFocalIdTree.emplace(coordinate, i);
    }

    ppFocalGroups.resize(ngroups);
    for (auto &g : ppFocalGroups) {
      g.second.focal = 0.0;
      g.second.pp = Point2();
    }
    for (int i = 0; i < ppFocalId2group.size(); i++) {
      auto &g = ppFocalGroups[ppFocalId2group[i]];
      g.first.insert(i);
      g.second.focal += ppFocalCandidates[i].focal;
      g.second.pp += ppFocalCandidates[i].pp;
    }
    for (auto &g : ppFocalGroups) {
      g.second.focal /= g.first.size();
      g.second.pp /= double(g.first.size());
    }

    std::sort(
        ppFocalGroups.begin(), ppFocalGroups.end(),
        [](auto &g1, auto &g2) { return g1.first.size() > g2.first.size(); });
  }



  //// [Orient Edges]
  // record edges
  std::vector<std::pair<HalfHandle, HalfHandle>> edge2hhs;
  std::vector<Line2> edge2line;
  HandledTable<HalfHandle, int> hh2edge(mesh2d.internalHalfEdges().size(), -1);
  int nedges = 0;
  {
    for (auto &h : mesh2d.halfedges()) {
      auto hh = h.topo.hd;
      auto oppohh = h.topo.opposite;
      if (hh2edge[hh] == -1 && hh2edge[oppohh] == -1) {
        hh2edge[hh] = hh2edge[oppohh] = nedges;
        nedges++;
        edge2hhs.push_back(MakeOrderedPair(hh, oppohh));
        edge2line.push_back(Line2(mesh2d.data(mesh2d.topo(hh).from()),
                                  mesh2d.data(mesh2d.topo(hh).to())));
      }
    }
    assert(edge2hhs.size() == nedges && edge2line.size() == nedges);
  }

  // collect edge intersections and
  // get vpPositions from the intersections
  std::vector<Point2> vpPositions;
  int nvps = 0;
  std::vector<std::vector<Scored<int>>> edge2OrderedVPAndAngles(nedges);
  {
    std::vector<Point2> intersections;
    std::vector<std::pair<int, int>> intersection2edges;
    intersections.reserve(nedges * (nedges - 1) / 2);
    intersection2edges.reserve(nedges * (nedges - 1) / 2);
    for (int i = 0; i < nedges; i++) {
      const Line2 &linei = edge2line[i];
      for (int j = i + 1; j < nedges; j++) {
        const Line2 &linej = edge2line[j];
        Point2 interp = Intersection(linei.ray(), linej.ray());
        if (std::min(Distance(interp, linei), Distance(interp, linej)) <=
            scale / 10.0) {
          continue;
        }
        intersections.push_back(interp);
        intersection2edges.emplace_back(i, j);
      }
    }
    assert(intersections.size() == intersection2edges.size());

    std::vector<int> intersection2Rawvp(intersections.size(), -1);
    RTreeMap<Point2, int> intersectionTree;
    for (int i = 0; i < intersections.size(); i++) {
      const double thres = scale / 30.0;
      auto &p = intersections[i];
      int nearestIntersectionId = -1;
      double minDist = thres;
      intersectionTree.search(
          BoundingBox(p).expand(thres * 2),
          [&nearestIntersectionId, &minDist,
           &p](const std::pair<Point2, int> &locationAndIntersectionId) {
            double dist = Distance(locationAndIntersectionId.first, p);
            if (dist < minDist) {
              minDist = dist;
              nearestIntersectionId = locationAndIntersectionId.second;
            }
            return true;
          });
      if (nearestIntersectionId != -1) {
        intersection2Rawvp[i] = intersection2Rawvp[nearestIntersectionId];
      } else {
        intersection2Rawvp[i] = nvps++;
      }
      intersectionTree.emplace(p, i);
    }

    // further merge vps
    // if any two vps share two or more edges, merge them
    std::vector<std::set<int>> rawVP2edges(nvps);
    for (int i = 0; i < intersection2Rawvp.size(); i++) {
      int vpid = intersection2Rawvp[i];
      rawVP2edges[vpid].insert(i);
    }
    std::vector<std::set<int>> rawVp2rawVpShouldMerge(nvps);
    for (int vp1 = 0; vp1 < nvps; vp1++) {
      auto &edges1 = rawVP2edges[vp1];
      for (int vp2 = vp1 + 1; vp2 < nvps; vp2++) {
        auto &edges2 = rawVP2edges[vp2];
        std::set<int> commonEdges;
        std::set_intersection(edges1.begin(), edges1.end(), edges2.begin(),
                              edges2.end(),
                              std::inserter(commonEdges, commonEdges.begin()));
        if (commonEdges.size() >= 2) {
          rawVp2rawVpShouldMerge[vp1].insert(vp2);
          rawVp2rawVpShouldMerge[vp2].insert(vp1);
        }
      }
    }
    std::map<int, std::set<int>> newVP2rawVPs;
    std::vector<int> rawVP2newVP(nvps, -1);
    std::vector<int> rawVPIds(nvps);
    std::iota(rawVPIds.begin(), rawVPIds.end(), 0);
    nvps = ConnectedComponents(
        rawVPIds.begin(), rawVPIds.end(),
        [&rawVp2rawVpShouldMerge](int vp) -> const std::set<int> & {
          return rawVp2rawVpShouldMerge.at(vp);
        },
        [&newVP2rawVPs, &rawVP2newVP](int rawVP, int newVP) {
          newVP2rawVPs[newVP].insert(rawVP);
          rawVP2newVP[rawVP] = newVP;
        });

    vpPositions.resize(nvps, Origin<2>());
    std::vector<std::set<int>> vp2intersections(nvps);
    for (int i = 0; i < intersection2Rawvp.size(); i++) {
      int rawVP = intersection2Rawvp[i];
      int newVP = rawVP2newVP[rawVP];
      vpPositions[newVP] += intersections[i]; // TODO: what if some
                                              // intersections are oppsite far
                                              // points?
      vp2intersections[newVP].insert(i);
    }
    for (int i = 0; i < nvps; i++) {
      vpPositions[i] /= double(vp2intersections[i].size());
    }

    // initial edge vp bindings
    std::vector<std::map<int, double>> vp2edgeWithAngles(nvps);
    std::vector<bool> vpIsGood(nvps, true);
    for (int vp = 0; vp < nvps; vp++) {
      auto &vpPos = vpPositions[vp];
      for (int edge = 0; edge < nedges; edge++) {
        auto &line = edge2line[edge];
        double lambda = ProjectionOfPointOnLine(vpPos, line).ratio;
        static const double thres = 0.1;
        if (lambda >= -thres && lambda <= 1.0 + thres) {
          continue;
        }
        double angle =
            AngleBetweenUndirected(line.direction(), vpPos - line.center());
        static const double theta = DegreesToRadians(5); ////// TODO
        if (angle >= theta) {
          continue;
        }
        vp2edgeWithAngles[vp][edge] = angle;
      }
      vpIsGood[vp] = vp2edgeWithAngles[vp].size() >= 3;
    }

    if (false) {
      for (int i = 0; i < std::min(10, nvps); i++) {
        Image3ub im(cam.screenSize(), Vec3ub(255, 255, 255));
        auto canvas = gui::MakeCanvas(im);
        canvas.color(gui::LightGray);
        canvas.thickness(2);
        for (auto &line : edge2line) {
          canvas.add(line);
        }
        canvas.color(gui::Gray);
        canvas.thickness(1);
        for (auto &edgeWithAngle : vp2edgeWithAngles[i]) {
          canvas.add(edge2line[edgeWithAngle.first].ray());
        }
        canvas.color(gui::Black);
        for (auto &edgeWithAngle : vp2edgeWithAngles[i]) {
          canvas.add(edge2line[edgeWithAngle.first]);
        }
        canvas.show(0, "before removing bad vps: raw vp_" + std::to_string(i));
      }
    }

    // remove bad vps
    int newvp = 0;
    for (int oldvp = 0; oldvp < nvps; oldvp++) {
      if (vpIsGood[oldvp]) {
        // update vpPositions
        // update vp2edgeWithAngles
        vpPositions[newvp] = vpPositions[oldvp];
        vp2edgeWithAngles[newvp] = std::move(vp2edgeWithAngles[oldvp]);
        newvp++;
      }
    }
    nvps = newvp;
    vpPositions.resize(nvps);
    vp2edgeWithAngles.resize(nvps);

    // construct edge2OrderedVPs
    for (int vp = 0; vp < nvps; vp++) {
      for (auto &edgeAndAngle : vp2edgeWithAngles[vp]) {
        int edge = edgeAndAngle.first;
        double angle = edgeAndAngle.second;
        edge2OrderedVPAndAngles[edge].push_back(ScoreAs(vp, angle));
      }
    }
    for (auto &vpAndAngles : edge2OrderedVPAndAngles) {
      std::sort(vpAndAngles.begin(), vpAndAngles.end());
    }

    if (false) {
      for (int i = 0; i < std::min(10, nvps); i++) {
        Image3ub im(cam.screenSize(), Vec3ub(255, 255, 255));
        auto canvas = gui::MakeCanvas(im);
        canvas.color(gui::LightGray);
        canvas.thickness(2);
        for (auto &line : edge2line) {
          canvas.add(line);
        }
        canvas.color(gui::Gray);
        canvas.thickness(2);
        for (auto &edgeWithAngle : vp2edgeWithAngles[i]) {
          canvas.add(edge2line[edgeWithAngle.first].ray());
        }
        canvas.color(gui::Black);
        for (auto &edgeWithAngle : vp2edgeWithAngles[i]) {
          canvas.add(edge2line[edgeWithAngle.first]);
        }
        canvas.show(0, "raw vp_" + std::to_string(i));
      }
    }
  }

  // construct a factor graph to optimize edge-vp bindings

  FactorGraph fg;
  std::vector<FactorGraph::VarHandle> edge2vh(nedges);
  {
    for (int edge = 0; edge < nedges; edge++) {
      auto vc =
          fg.addVarCategory(edge2OrderedVPAndAngles[edge].size() + 1, 1.0);
      edge2vh[edge] = fg.addVar(vc);
    }

    // potential 1: the edge should bind to some vp, should prefer better scored
    for (int edge = 0; edge < nedges; edge++) {
      auto vh = edge2vh[edge];
      auto &relatedVPAndAngles = edge2OrderedVPAndAngles[edge];
      auto fc = fg.addFactorCategory(
          [&relatedVPAndAngles, nedges](const int *varlabels, size_t nvar,
                                        FactorGraph::FactorCategoryId fcid,
                                        void *givenData) -> double {
            assert(nvar == 1);
            int label = varlabels[0];
            assert(label <= relatedVPAndAngles.size());
            const double K = 50.0 / nedges;
            if (label == relatedVPAndAngles.size()) { // not bind to any vp
              return K;
            }
            double angle = relatedVPAndAngles[label].score;
            assert(!IsInfOrNaN(angle));
            return (1.0 - Gaussian(angle, DegreesToRadians(3))) * K;
          },
          1.0);
      fg.addFactor({vh}, fc);
    }

    // potential 2: two adjacent edges should not bind to a near vp
    int ncorners = 0;
    for (auto &f : mesh2d.faces()) {
      ncorners += f.topo.halfedges.size();
    }
    for (auto &f : mesh2d.faces()) {
      auto &hhs = f.topo.halfedges;
      for (int i = 0; i < hhs.size(); i++) {
        auto hh1 = hhs[i];
        auto hh2 = hhs[(i + 1) % hhs.size()];
        int edge1 = hh2edge[hh1];
        int edge2 = hh2edge[hh2];
        auto &relatedVPAndAngles1 = edge2OrderedVPAndAngles[edge1];
        auto &relatedVPAndAngles2 = edge2OrderedVPAndAngles[edge2];
        auto vh1 = edge2vh[edge1];
        auto vh2 = edge2vh[edge2];
        auto fc = fg.addFactorCategory(
            [edge1, edge2, &relatedVPAndAngles1, &relatedVPAndAngles2,
             &vpPositions, ncorners, scale](const int *varlabels, size_t nvar,
                                            FactorGraph::FactorCategoryId fcid,
                                            void *givenData) -> double {
              assert(nvar == 2);
              int bindedVP1 =
                  varlabels[0] == relatedVPAndAngles1.size()
                      ? -1
                      : (relatedVPAndAngles1[varlabels[0]].component);
              int bindedVP2 =
                  varlabels[1] == relatedVPAndAngles2.size()
                      ? -1
                      : (relatedVPAndAngles2[varlabels[1]].component);
              if (bindedVP1 == -1 || bindedVP2 == -1) {
                return 0;
              }
              auto &vpPos1 = vpPositions[bindedVP1];
              auto &vpPos2 = vpPositions[bindedVP2];
              const double thres = scale / 10.0;
              const double K = 10.0 / ncorners;
              if (Distance(vpPos1, vpPos2) < thres) { // todo
                return K;
              }
              return 0.0;
            },
            1.0);
        fg.addFactor({vh1, vh2}, fc);
      }
    }

    // potential 3: the vpPositions of edges sharing a same face should lie on
    // the same
    // line (the vanishing line of the face)
    int ntris = 0;
    for (auto &f : mesh2d.faces()) {
      auto &hhs = f.topo.halfedges;
      if (hhs.size() <= 3) {
        continue;
      }
      ntris = hhs.size() > 4 ? (2 * hhs.size()) : hhs.size();
    }
    for (auto &f : mesh2d.faces()) {
      auto &hhs = f.topo.halfedges;
      if (hhs.size() <= 3) {
        continue;
      }
      for (int i = 0; i < hhs.size(); i++) {
        int maxGap = hhs.size() > 4 ? 2 : 1;
        for (int gap = 1; gap <= maxGap; gap++) {
          int prevEdge = hh2edge[hhs[(i + hhs.size() - gap) % hhs.size()]];
          int edge = hh2edge[hhs[i]];
          int nextEdge = hh2edge[hhs[(i + gap) % hhs.size()]];
          auto fc = fg.addFactorCategory(
              [&edge2OrderedVPAndAngles, &vpPositions, prevEdge, edge, nextEdge,
               ntris](const int *varlabels, size_t nvar,
                      FactorGraph::FactorCategoryId fcid,
                      void *givenData) -> double {
                assert(nvar == 3);
                int vp1 =
                    varlabels[0] == edge2OrderedVPAndAngles[prevEdge].size()
                        ? -1
                        : (edge2OrderedVPAndAngles[prevEdge][varlabels[0]]
                               .component);
                if (vp1 == -1) {
                  return 0.0;
                }
                int vp2 = varlabels[1] == edge2OrderedVPAndAngles[edge].size()
                              ? -1
                              : (edge2OrderedVPAndAngles[edge][varlabels[1]]
                                     .component);
                if (vp2 == -1) {
                  return 0.0;
                }
                int vp3 =
                    varlabels[2] == edge2OrderedVPAndAngles[nextEdge].size()
                        ? -1
                        : (edge2OrderedVPAndAngles[nextEdge][varlabels[2]]
                               .component);
                if (vp3 == -1) {
                  return 0.0;
                }
                if (vp1 == vp2 || vp2 == vp3 || vp1 == vp3) {
                  return 0.0;
                }
                double angle =
                    AngleBetweenUndirected(vpPositions[vp1] - vpPositions[vp2],
                                           vpPositions[vp3] - vpPositions[vp2]);
                assert(!IsInfOrNaN(angle));
                const double K = 30.0 / ntris;
                return (1.0 - Gaussian(angle, DegreesToRadians(10))) * K;
              },
              1.0);
          fg.addFactor({edge2vh[prevEdge], edge2vh[edge], edge2vh[nextEdge]},
                       fc);
        }
      }
    }

    //// potential 4: a vp is good only when edges are bound to it!
    //// a vp should be with either no edges or >= 3 edges
    // for (int vpid = 0; vpid < nvps; vpid++) {
    //  std::vector<FactorGraph::VarHandle> relatedVhs;
    //  relatedVhs.reserve(vp2edges[vpid].size());
    //  for (int edge : vp2edges[vpid]) {
    //    relatedVhs.push_back(edge2vh[edge]);
    //  }
    //  auto fc = fg.addFactorCategory(
    //      [&edge2OrderedVPAndAngles, vpid, &vp2edges](
    //          const int *varlabels, size_t nvar,
    //          FactorGraph::FactorCategoryId fcid, void *givenData) -> double {
    //        int bindedEdges = 0;
    //        auto &relatedEdges = vp2edges[vpid];
    //        assert(nvar == relatedEdges.size());
    //        for (int i = 0; i < relatedEdges.size(); i++) {
    //          int edge = relatedEdges[i];
    //          int edgeLabel = varlabels[i];
    //          if (edgeLabel == edge2OrderedVPAndAngles[edge].size()) {
    //            continue; // not bound to any vp
    //          }
    //          int edgeBindedVPId =
    //          edge2OrderedVPAndAngles[edge][edgeLabel].component;
    //          if (edgeBindedVPId == vpid) {
    //            bindedEdges++;
    //          }
    //        }
    //        // todo
    //        if (bindedEdges == 1 || bindedEdges == 2) {
    //          return 10.0;
    //        }
    //        return 0.0;
    //      },
    //      1.0);
    //  fg.addFactor(relatedVhs.begin(), relatedVhs.end(), fc);
    //}
  }

  // solve the factor graph
  auto result =
      fg.solve(5, 1, [](int epoch, double energy, double denergy,
                        const FactorGraph::ResultTable &results) -> bool {
        Println("epoch: ", epoch, "  energy: ", energy);
        return true;
      });

  std::vector<int> optimizedEdge2VP(nedges, -1);
  std::vector<std::vector<int>> optimizedVP2Edges(nvps);
  for (int edge = 0; edge < nedges; edge++) {
    int id = result[edge2vh[edge]];
    if (id == edge2OrderedVPAndAngles[edge].size()) {
        continue;
    }
    optimizedEdge2VP[edge] = edge2OrderedVPAndAngles[edge][id].component;
    optimizedVP2Edges[optimizedEdge2VP[edge]].push_back(edge);
  }

  // invalidate the edge bindings for vps who have only 1 or 2 edges
  for (int vp = 0; vp < nvps; vp++) {
    if (optimizedVP2Edges[vp].size() <= 2) {
      for (int edge : optimizedVP2Edges[vp]) {
        optimizedEdge2VP[edge] = -1;
      }
      optimizedVP2Edges[vp].clear();
    }
  }

  if (true) { // show line classification results
    for (int i = 0; i < nvps; i++) {
      if (optimizedVP2Edges[i].empty()) {
        continue;
      }
      Image3ub im(cam.screenSize(), Vec3ub(255, 255, 255));
      auto canvas = gui::MakeCanvas(im);
      canvas.color(gui::LightGray);
      canvas.thickness(2);
      for (auto &line : edge2line) {
        canvas.add(line);
      }
      canvas.color(gui::Gray);
      canvas.thickness(2);
      for (int edge : optimizedVP2Edges[i]) {
        canvas.add(edge2line[edge].ray());
      }
      canvas.color(gui::Black);
      for (int edge : optimizedVP2Edges[i]) {
        canvas.add(edge2line[edge]);
      }
      canvas.show(0, "optimized vp_" + std::to_string(i));
    }
  }

  

  //// []




  for (int configId = 0;
       configId < std::min(5ull, ppFocalGroups.size()) &&
       ppFocalGroups[configId].first.size() * 10 >= ppFocalCandidates.size();
       configId++) {

    double focal = ppFocalGroups[configId].second.focal;
    auto &pp = ppFocalGroups[configId].second.pp;

    PerspectiveCamera curCam(cam.screenWidth(), cam.screenHeight(), pp, focal);
    std::vector<Vec3> vp2dir(nvps);
    for (int i = 0; i < nvps; i++) {
      vp2dir[i] = curCam.direction(vpPositions[i]);
    }

    
  }
}