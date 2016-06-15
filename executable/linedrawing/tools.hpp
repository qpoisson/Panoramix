#pragma once

#include "basic_types.hpp"
#include "iterators.hpp"
#include "utility.hpp"

namespace pano {
namespace experimental {

using namespace ::pano::core;

// HalfCubeMap
struct CubeMapLocation {
  int panel_id;
  Pixel pixel;
	CubeMapLocation(){}
	CubeMapLocation(int pid, const Pixel & p, size_t sz);
	Vec3 direction(size_t sz) const;

  static CubeMapLocation FromDirection(size_t sz, const Vec3 &dir);
};
template <class T> struct HalfCubeMap {
  Image_<T> panels[3];
  explicit HalfCubeMap(size_t sz)
      : panels{Image_<T>::zeros(sz, sz), Image_<T>::zeros(sz, sz),
               Image_<T>::zeros(sz, sz)} {}
  const T &at(const CubeMapLocation &loc) const {
    return panels[loc.panel_id](loc.pixel);
  }
  const T &operator()(const CubeMapLocation &loc) const { return at(loc); }
  T &operator()(const CubeMapLocation &loc) {
    return panels[loc.panel_id](loc.pixel);
  }
  const T &at(const Vec3 &dir) const {
    return at(CubeMapLocation::FromDirection(panels[0].cols, dir));
  }
  const T &operator()(const Vec3 &dir) const { return at(dir); }
  T &operator()(const Vec3 &dir) {
    return (*this)(CubeMapLocation::FromDirection(panels[0].cols, dir));
  }
};

// BinaryRelationTable
template <class T> struct BinaryRelationTable {
  std::vector<T> relations;
  size_t nelements;
  explicit BinaryRelationTable(size_t n, const T &v)
      : nelements(n), relations(n * (n - 1) / 2, v) {}
  decltype(auto) operator()(int i, int j) const {
    if (i == j) {
      return T();
    }
    int offset = i < j ? (j * (j - 1) / 2 + i) : (i * (i - 1) / 2 + j);
    return relations[offset];
  }
  decltype(auto) operator()(int i, int j) {
    assert(i != j);
    int offset = i < j ? (j * (j - 1) / 2 + i) : (i * (i - 1) / 2 + j);
    return relations[offset];
  }
  constexpr auto nonZeroNeighbors(int i) const {
    return MakeConditionalRange(MakeIotaRange<int>(nelements),
                                [this, i](int ind) { return (*this)(i, ind); });
  }
};



// DecomposeFaces
// assume all internal faces are already collected in face2verts
std::vector<std::set<int>>
DecomposeFaces(const std::vector<std::vector<int>> &face2verts,
               const std::vector<Point2> &vert2pos);

// CameraParam
struct CameraParam {
  Point2 pp;
  double focal;
  template <class ArchiverT> void serialize(ArchiverT &ar) { ar(pp, focal); }
};

// CalibrateCamera
std::vector<CameraParam>
CalibrateCamera(const Box2 &box, const std::vector<std::set<int>> &face_groups,
                std::function<std::vector<Chain2>(int face)> face2chain_fun,
                int k = std::numeric_limits<int>::max());

// BindPointsToLines
std::vector<std::set<int>> BindPointsToLines(const std::vector<Point2> &points,
                                             const std::vector<Line2> &lines,
                                             double angle_thres);
std::vector<std::set<int>> BindPointsToLines(const std::vector<Vec3> &points,
                                             const std::vector<Line3> &lines,
                                             double angle_thres);
std::vector<std::set<int>> BindPointsToLines(const std::vector<Point2> &points,
                                             const std::vector<Line2> &lines,
                                             const CameraParam &cam_param,
                                             double angle_thres);

// CollectLineIntersections
std::vector<Vec3>
CollectLineIntersections(const std::vector<Line3> &lines,
                         std::vector<std::pair<int, int>> *line_ids = nullptr);

// CollectVanishingPoints
struct CollectVanishingPointsParam { // best params so far
  double angle_thres_phase1 = DegreesToRadians(2);
  double angle_thres_phase2 = DegreesToRadians(0.1);
  double angle_thres_phase3 = DegreesToRadians(8);
  int max_iters = std::numeric_limits<int>::max();
  bool use_mean_shift_merge_phase1 = false;
};
//std::vector<Point2> CollectVanishingPoints(
//    const std::vector<Line2> &lines, const CameraParam &cam_param,
//    const CollectVanishingPointsParam &param = CollectVanishingPointsParam());
std::vector<Vec3> CollectVanishingPoints(
    const std::vector<Line3> &lines,
    const CollectVanishingPointsParam &param = CollectVanishingPointsParam());

// MergeColinearLines
std::vector<Line2>
MergeColinearLines(const std::vector<Line2> &lines,
                   const CameraParam &cam_param, double angle_thres,
                   std::vector<int> *oldline2newline = nullptr);

//// EstimateParallelism
//std::map<std::pair<int, int>, double>
//EstimateParallelism(const std::vector<Line2> &lines,
//                    const CameraParam &cam_param, double angle_thres);

// EstimateEdgeOrientations
struct EstimateEdgeOrientationsParam { // best params so far
  double angle_thres_allowed_vp_line_deviation = DegreesToRadians(10);
  double angle_thres_judging_colinearility = DegreesToRadians(1);
  double angle_thres_distinguishing_vps = DegreesToRadians(2);
	double angle_thres_juding_orthogonality = DegreesToRadians(10);
  double angle_thres_juding_coplanarity = DegreesToRadians(10);
  double coeff_vp_line_fitness = 50.0;
  double coeff_noncolinear_adj_line_exlusiveness = 10.0;
	double coeff_line_pair_orthogonality = 20.0;
  double coeff_line_triplet_coplanar = 30.0;
  int vp_min_degree = 3;
  int solve_max_iter = 5;
};
//std::vector<int> EstimateEdgeOrientations(
//    const std::vector<Line2> &lines, const std::vector<Point2> &vps,
//    const std::vector<std::vector<int>> &face2ordered_lines, double focal,
//    const Point2 &pp, const EstimateEdgeOrientationsParam &param =
//                          EstimateEdgeOrientationsParam());
std::vector<int> EstimateEdgeOrientations(
    const std::vector<Line3> &lines, const std::vector<Vec3> &vps,
	  const std::vector<std::pair<int, int>> & adjacent_line_pairs,
    const std::vector<std::vector<int>> &coplanar_ordered_lines,
    const EstimateEdgeOrientationsParam &param =
        EstimateEdgeOrientationsParam());

// PlaneConstraint
struct PlaneConstraint {
  std::vector<int> verts;
  DenseMatd P; // the matrix P_i in my cvpr16 paper
  template <class ArchiverT> void serialize(ArchiverT &ar) { ar(verts, P); }
};
DenseMatd MakePlaneMatrix();
DenseMatd MakePlaneMatrixAlongDirection(const Vec3 & dir);
DenseMatd MakePlaneMatrixTowardDirection(const Vec3 & dir);

// InferenceFunctors
struct Inferencer {
  virtual size_t nvars() const = 0;
  virtual Vec3 getPlaneEquation(int cons, const DenseMatd &variables) const = 0;
  virtual double getInversedDepth(int vert,
                                  const DenseMatd &variables) const = 0;
  virtual DenseMatd
  recoverVariables(const std::vector<double> &vert2inversed_depths) const = 0;
};
// GenerateInferenceFunctors
std::unique_ptr<Inferencer>
GenerateInferenceFunctors(const std::vector<PlaneConstraint> &constraints,
                          const std::vector<Vec3> &vert2dir, int root_vert = 0,
                          std::vector<int> *fundamental_verts = nullptr);


// The Energy Terms
std::vector<double> AnglesBetweenAdjacentEdges(
    const std::vector<Vec3> &vert2dir,
    const std::vector<std::vector<int>> &face2verts, const DenseMatd &variables,
    const Inferencer &infer,
    std::function<bool(int v1, int v2)> edge_selected = nullptr,
    std::function<bool(int face)> face_selected = nullptr);

std::vector<double> AnglesBetweenAdjacentFaces(
    size_t nfaces, const std::vector<std::set<int>> &edge2faces,
    const DenseMatd &variables, const Inferencer &infer,
    std::function<bool(int face)> face_selected = nullptr);

std::vector<double> AnglesBetweenAdjacentFaces(
    size_t nfaces, const std::vector<std::set<int>> &edge2faces,
    const DenseMatd &variables, const Inferencer &infer,
    const std::map<std::pair<int, int>, bool> &faces_overlap,
    std::function<bool(int face)> face_selected = nullptr);

template <class IterT> auto MeanSquaredDeviation(IterT begin, IterT end) {
  using T = typename std::iterator_traits<IterT>::value_type;
  auto n = std::distance(begin, end);
  T mean = std::accumulate(begin, end, T(0)) / double(n);
  T msd = 0;
  while (begin != end) {
    msd += Square(*begin - mean);
    ++ begin;
  }
  return msd / double(n);
}

template <class ContT> auto MeanSquaredDeviationOfContainer(const ContT &cont) {
  return MeanSquaredDeviation(std::begin(cont), std::end(cont));
}

template <class IterT, class T>
auto MeanSquaredDeviation(IterT begin, IterT end,
                          std::initializer_list<T> vals) {
  auto n = std::distance(begin, end);
  T msd = 0;
  while (begin != end) {
    auto min_dev = std::numeric_limits<T>::max();
    for (auto val : vals) {
      auto dev = Square(*begin - val);
      if (dev < min_dev) {
        min_dev = dev;
      }
    }
    msd += min_dev;
    ++ begin;
  }
  return msd / double(n);
}

template <class ContT, class T>
auto MeanSquaredDeviationOfContainer(const ContT &cont,
                                     std::initializer_list<T> vals) {
  return MeanSquaredDeviation(std::begin(cont), std::end(cont), vals);
}

// PerformReconstruction
struct PerformReconstructionParam {
  int max_iters = 100;
};
double PerformReconstruction(
    const std::vector<PlaneConstraint> &constraints,
    const std::vector<Vec3> &vert2dir, int root_vert,
    std::function<double(const Inferencer &infer, const DenseMatd &variables,
                         const std::vector<Vec3> &vert2dir)>
        energy_fun,
    std::default_random_engine &rng, std::vector<Point3> &vert2pos,
    std::vector<int> *fundamental_verts_ptr = nullptr,
    const PerformReconstructionParam &param = PerformReconstructionParam());
}
}