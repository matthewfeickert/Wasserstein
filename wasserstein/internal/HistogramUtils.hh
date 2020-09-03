//------------------------------------------------------------------------
// This file is part of Wasserstein, a C++ library with a Python wrapper
// that computes the Wasserstein/EMD distance. If you use it for academic
// research, please cite or acknowledge the following works:
//
//   - Komiske, Metodiev, Thaler (2019) arXiv:1902.02346
//       https://doi.org/10.1103/PhysRevLett.123.041801
//   - Boneel, van de Panne, Paris, Heidrich (2011)
//       https://doi.org/10.1145/2070781.2024192
//   - LEMON graph library https://lemon.cs.elte.hu/trac/lemon
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//------------------------------------------------------------------------

#ifndef WASSERSTEIN_HISTOGRAMUTILS_HH
#define WASSERSTEIN_HISTOGRAMUTILS_HH

// C++ standard library
#include <cmath>
#include <iomanip>
#include <utility>

// Boost histogram headers
#include <boost/histogram.hpp>
#ifdef BOOST_HISTOGRAM_SERIALIZATION_HPP
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#endif

#include "EMDUtils.hh"

BEGIN_WASSERSTEIN_NAMESPACE

namespace hist {

// gets bin centers from an axis
template<class Axis>
inline std::vector<double> get_bin_centers(const Axis & axis) {

  std::vector<double> bin_centers_vec(axis.size());
  for (int i = 0; i < axis.size(); i++)
    bin_centers_vec[i] = axis.bin(i).center();

  return bin_centers_vec;
}

// gets bin edges from an axis
template<class Axis>
inline std::vector<double> get_bin_edges(const Axis & axis) {

  if (axis.size() == 0) return std::vector<double>();

  std::vector<double> bins_vec(axis.size() + 1);
  bins_vec[0] = axis.bin(0).lower();
  for (int i = 0; i < axis.size(); i++)
    bins_vec[i+1] = axis.bin(i).upper();

  return bins_vec;
}

template<class Axis>
std::size_t get_1d_hist_size(const Axis & axis, bool overflows) {
  return axis.size() + (overflows ? 2 : 0);
}

template<class Hist>
std::pair<std::vector<double>, std::vector<double>>
get_1d_hist(const Hist & hist, bool overflows) {

  // setup containers to hold histogram values
  long long size(hist::get_1d_hist_size(hist.template axis<0>(), overflows)),
            nbins(hist.template axis<0>().size());
  std::vector<double> hist_vals(size), hist_errs(size);

  for (long long i = (overflows ? -1 : 0), a = 0;
       i < nbins + (overflows ? 1 : 0); i++, a++) {
    const auto & x(hist.at(i));
    hist_vals[a] = x.value();
    hist_errs[a] = std::sqrt(x.variance());
  }

  return std::make_pair(hist_vals, hist_errs);
}

template<class Transform>
std::string name_transform() {
  if (std::is_same<Transform, boost::histogram::axis::transform::log>::value)
    return "log";
  else if (std::is_same<Transform, boost::histogram::axis::transform::id>::value)
    return "id";
  else if (std::is_same<Transform, boost::histogram::axis::transform::sqrt>::value)
    return "sqrt";
  else if (std::is_same<Transform, boost::histogram::axis::transform::pow>::value)
    return "pow";
  else
    return "unknown";
}

template<class Axis>
inline std::string print_axis(const Axis & axis) {

  std::ostringstream oss;
  oss << std::setprecision(16);

  for (int i = 0; i < axis.size(); i++) {
    const auto & bin(axis.bin(i));
    oss << i << " : " << bin.lower() << ' ' << bin.center() << ' ' << bin.upper() << '\n';
  }
  oss << '\n';

  return oss.str();
}

template<class Hist>
inline std::string print_1d_hist(const Hist & hist) {

  std::ostringstream oss;
  oss << std::setprecision(16);

  for (const auto && x : boost::histogram::indexed(hist, boost::histogram::coverage::all)) {
    const auto & wsum(*x);
    oss << x.index(0) << " : " << wsum.value() << ' ' << std::sqrt(wsum.variance()) << '\n';
  }
  oss << '\n';

  return oss.str();
}

} // namespace hist

// use boost::histogram::axis::transform::id if no axis transform desired
template<class T = boost::histogram::axis::transform::id>
class Histogram1DHandler : public ExternalEMDHandler {
public:
  typedef T Transform;
  typedef boost::histogram::axis::regular<double, Transform> Axis;

protected:

#ifndef SWIG_PREPROCESSOR
  Axis axis_;
public:
  typedef decltype(boost::histogram::make_weighted_histogram(axis_)) Hist;
protected:
  Hist hist_;
#endif

public:

  Histogram1DHandler(unsigned int nbins, double axis_min, double axis_max) :
    axis_(nbins, axis_min, axis_max),
    hist_(boost::histogram::make_weighted_histogram(axis_))
  {
    if (nbins == 0)
      throw std::invalid_argument("Number of histogram bins should be a positive integer");
    if (axis_min >= axis_max)
      throw std::invalid_argument("axis_min should be less than axis_max");
  }

  Histogram1DHandler() {}
  virtual ~Histogram1DHandler() {}

  unsigned int nbins() const { return axis_.size(); }
  std::string description() const {
    std::ostringstream oss;
    oss << std::setprecision(8)
        << "  ExternalEMDHandler - " << name() << '\n'
        << "    range - [" << axis_.bin(0).lower() << ", " << axis_.bin(axis_.size() - 1).upper() << ")\n"
        << "    bins - " << nbins() << '\n'
        << "    axis_transform - " << hist::name_transform<Transform>() << '\n';

    return oss.str();
  }

  // SWIG preprocessor complains about these, so hide them
  #ifndef SWIG_PREPROCESSOR
  Hist & hist() { return hist_; }
  Axis & axis() { return axis_; }
  #endif

  std::vector<double> bin_centers() const { return hist::get_bin_centers(axis_); }
  std::vector<double> bin_edges() const { return hist::get_bin_edges(axis_); }

  std::pair<std::vector<double>, std::vector<double>> hist_vals_errs(bool overflows = true) const {
    return hist::get_1d_hist(hist_, overflows);
  }

  std::string print_axis() const { return hist::print_axis(axis_); }
  std::string print_hist() const { return hist::print_1d_hist(hist_); }

#ifdef BOOST_HISTOGRAM_SERIALIZATION_HPP
  
  void load(std::istream & is) {
    boost::archive::text_iarchive ia(is);
    ia >> axis_ >> hist_;
  }

  void save(std::ostream & os) {
    boost::archive::text_oarchive oa(os);
    oa << axis_;
    oa << hist_;
  }

#endif // BOOST_HISTOGRAM_SERIALIZATION_HPP

protected:

  void handle(double emd, double weight) {
    hist_(boost::histogram::weight(weight), emd);
  }

  virtual std::string name() const { return "Histogram1DHandler"; }

}; // Histogram1DHandler

END_WASSERSTEIN_NAMESPACE

#endif // WASSERSTEIN_HISTOGRAMUTILS_HH