/*
Author(s): Djordje Vukcevic, Sven Schneider
Institute: Hochschule Bonn-Rhein-Sieg

Copyright (c) [2019]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef MOVING_VARIANCE_HPP_
#define MOVING_VARIANCE_HPP_
#include <Eigen/Core>
#include <iostream>
#include <vector>
#include <list>
#include <stdlib.h>     /* abs */

class moving_variance
{
  public:
    moving_variance(const int window_size, const int num_dimensions);
    ~moving_variance(){};

    Eigen::VectorXd update(const Eigen::VectorXd &state);
    double update(const int dimension, const double state);

    void clear();
    void clear(const int dimension);

    Eigen::VectorXd get_variance();
    double get_variance(const int dimension);

    Eigen::VectorXd get_mean();
    double get_mean(const int dimension);

  private:
    const int DIMENSIONS_, WINDOW_SIZE_;
    std::vector<int> indexes_;
    Eigen::VectorXd means_, variances_;
    std::vector<std::list<double>> windows_;
};
#endif /* MOVING_VARIANCE_HPP_*/