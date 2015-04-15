/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2011, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/** \author Jia Pan */


#define BOOST_TEST_MODULE "FCL_GEOMETRIC_SHAPES"
#include <boost/test/unit_test.hpp>

#include "fcl/narrowphase/narrowphase.h"
#include "fcl/collision.h"
#include "test_fcl_utility.h"
#include "fcl/ccd/motion.h"
#include <iostream>

using namespace fcl;

FCL_REAL extents [6] = {0, 0, 0, 10, 10, 10};

GJKSolver_libccd solver1;
GJKSolver_indep solver2;

#define BOOST_CHECK_FALSE(p) BOOST_CHECK(!(p))

BOOST_AUTO_TEST_CASE(gjkcache)
{
  Cylinder s1(5, 10);
  Cone s2(5, 10);

  CollisionRequest request;
  request.enable_cached_gjk_guess = true;
  request.gjk_solver_type = GST_INDEP;

  TranslationMotion motion(Transform3f(Vec3f(-20.0, -20.0, -20.0)), Transform3f(Vec3f(20.0, 20.0, 20.0)));

  int N = 1000;  
  FCL_REAL dt = 1.0 / (N - 1);

  /// test exploiting spatial coherence
  Timer timer1;
  timer1.start();
  std::vector<bool> result1(N);
  for(int i = 0; i < N; ++i)
  {
    motion.integrate(dt * i);
    Transform3f tf;
    motion.getCurrentTransform(tf);

    CollisionResult result;

    collide(&s1, Transform3f(), &s2, tf, request, result);
    result1[i] = result.isCollision();
    request.cached_gjk_guess = result.cached_gjk_guess; // use cached guess
  }

  timer1.stop();

  /// test without exploiting spatial coherence
  Timer timer2;
  timer2.start();
  std::vector<bool> result2(N);
  request.enable_cached_gjk_guess = false;
  for(int i = 0; i < N; ++i)
  {
    motion.integrate(dt * i);
    Transform3f tf;
    motion.getCurrentTransform(tf);

    CollisionResult result;

    collide(&s1, Transform3f(), &s2, tf, request, result);
    result2[i] = result.isCollision();
  }

  timer2.stop();

  std::cout << timer1.getElapsedTime() << " " << timer2.getElapsedTime() << std::endl;

  for(std::size_t i = 0; i < result1.size(); ++i)
  {
    BOOST_CHECK(result1[i] == result2[i]);
  }
}

template <typename S1, typename S2>
void printComparisonError(const std::string& comparison_type,
                          const S1& s1, const Transform3f& tf1,
                          const S2& s2, const Transform3f& tf2,
                          GJKSolverType solver_type,
                          const Vec3f& contact_or_normal,
                          const Vec3f& expected_contact_or_normal,
                          bool check_opposite_normal,
                          FCL_REAL tol)
{
  std::cout << "Disagreement between " << comparison_type
            << " and expected_" << comparison_type << " for "
            << getNodeTypeName(s1.getNodeType()) << " and "
            << getNodeTypeName(s2.getNodeType()) << " with '"
            << getGJKSolverName(solver_type) << "' solver." << std::endl
            << "tf1.quaternion: " << tf1.getQuatRotation() << std::endl
            << "tf1.translation: " << tf1.getTranslation() << std::endl
            << "tf2.quaternion: " << tf2.getQuatRotation() << std::endl
            << "tf2.translation: " << tf2.getTranslation() << std::endl
            << comparison_type << ": " << contact_or_normal << std::endl
            << "expected_" << comparison_type << ": " << expected_contact_or_normal;

  if (check_opposite_normal)
    std::cout << " or " << -expected_contact_or_normal;

  std::cout << std::endl
            << "difference: " << (contact_or_normal - expected_contact_or_normal).norm() << std::endl
            << "tolerance: " << tol << std::endl;
}

template <typename S1, typename S2>
void printComparisonError(const std::string& comparison_type,
                          const S1& s1, const Transform3f& tf1,
                          const S2& s2, const Transform3f& tf2,
                          GJKSolverType solver_type,
                          FCL_REAL depth,
                          FCL_REAL expected_depth,
                          FCL_REAL tol)
{
  std::cout << "Disagreement between " << comparison_type
            << " and expected_" << comparison_type << " for "
            << getNodeTypeName(s1.getNodeType()) << " and "
            << getNodeTypeName(s2.getNodeType()) << " with '"
            << getGJKSolverName(solver_type) << "' solver." << std::endl
            << "tf1.quaternion: " << tf1.getQuatRotation() << std::endl
            << "tf1.translation: " << tf1.getTranslation() << std::endl
            << "tf2.quaternion: " << tf2.getQuatRotation() << std::endl
            << "tf2.translation: " << tf2.getTranslation() << std::endl
            << "depth: " << depth << std::endl
            << "expected_depth: " << expected_depth << std::endl
            << "difference: " << std::fabs(depth - expected_depth) << std::endl
            << "tolerance: " << tol << std::endl;
}

template <typename S1, typename S2>
void compareContact(const S1& s1, const Transform3f& tf1,
                    const S2& s2, const Transform3f& tf2,
                    GJKSolverType solver_type,
                    const Vec3f& contact, Vec3f* expected_point,
                    FCL_REAL depth, FCL_REAL* expected_depth,
                    const Vec3f& normal, Vec3f* expected_normal, bool check_opposite_normal,
                    FCL_REAL tol)
{
  if (expected_point)
  {
    bool contact_equal = contact.equal(*expected_point, tol);
    BOOST_CHECK(contact_equal);
    if (!contact_equal)
      printComparisonError("contact", s1, tf1, s2, tf2, solver_type, contact, *expected_point, false, tol);
  }

  if (expected_depth)
  {
    bool depth_equal = std::fabs(depth - *expected_depth) < tol;
    BOOST_CHECK(depth_equal);
    if (!depth_equal)
      printComparisonError("depth", s1, tf1, s2, tf2, solver_type, depth, *expected_depth, tol);
  }

  if (expected_normal)
  {
    bool normal_equal = normal.equal(*expected_normal, tol);

    if (!normal_equal && check_opposite_normal)
      normal_equal = normal.equal(-(*expected_normal), tol);

    BOOST_CHECK(normal_equal);
    if (!normal_equal)
      printComparisonError("normal", s1, tf1, s2, tf2, solver_type, normal, *expected_normal, check_opposite_normal, tol);
  }
}

template <typename S1, typename S2>
void testShapeInersection(const S1& s1, const Transform3f& tf1,
                          const S2& s2, const Transform3f& tf2,
                          GJKSolverType solver_type,
                          bool expected_res,
                          Vec3f* expected_point = NULL,
                          FCL_REAL* expected_depth = NULL,
                          Vec3f* expected_normal = NULL,
                          bool check_opposite_normal = false,
                          FCL_REAL tol = 1e-9)
{
  CollisionRequest request;
  request.gjk_solver_type = solver_type;
  CollisionResult result;

  Vec3f contact;
  FCL_REAL depth;
  Vec3f normal;  // normal direction should be from object 1 to object 2
  bool res;

  if (solver_type == GST_LIBCCD)
  {
    res = solver1.shapeIntersect(s1, tf1, s2, tf2, NULL, NULL, NULL);
  }
  else if (solver_type == GST_INDEP)
  {
    res = solver2.shapeIntersect(s1, tf1, s2, tf2, NULL, NULL, NULL);
  }
  else
  {
    std::cerr << "Invalid GJK solver. Test aborted." << std::endl;
    return;
  }
  BOOST_CHECK_EQUAL(res, expected_res);

  if (solver_type == GST_LIBCCD)
  {
    res = solver1.shapeIntersect(s1, tf1, s2, tf2, &contact, &depth, &normal);
  }
  else if (solver_type == GST_INDEP)
  {
    res = solver2.shapeIntersect(s1, tf1, s2, tf2, &contact, &depth, &normal);
  }
  else
  {
    std::cerr << "Invalid GJK solver. Test aborted." << std::endl;
    return;
  }
  BOOST_CHECK_EQUAL(res, expected_res);
  if (expected_res)
    compareContact(s1, tf1, s2, tf2, solver_type, contact, expected_point, depth, expected_depth, normal, expected_normal, check_opposite_normal, tol);

  if (s1.getNodeType() == GEOM_HALFSPACE || s2.getNodeType() == GEOM_HALFSPACE)
  {
    std::cout << "Abort test since Halfspace is not registered to the collision matrix. "
              << "Please see issue #57." << std::endl;
    return;
  }

  request.enable_contact = false;
  result.clear();
  res = (collide(&s1, tf1, &s2, tf2, request, result) > 0);
  BOOST_CHECK_EQUAL(res, expected_res);

  request.enable_contact = true;
  result.clear();
  res = (collide(&s1, tf1, &s2, tf2, request, result) > 0);
  BOOST_CHECK_EQUAL(res, expected_res);
  if (expected_res)
  {
    BOOST_CHECK_EQUAL(result.numContacts(), 1);
    if (result.numContacts() == 1)
    {
      Contact contact = result.getContact(0);
      compareContact(s1, tf1, s2, tf2, solver_type, contact.pos, expected_point, contact.penetration_depth, expected_depth, contact.normal, expected_normal, check_opposite_normal, tol);
    }
  }
}

BOOST_AUTO_TEST_CASE(shapeIntersection_spheresphere)
{
  Sphere s1(20);
  Sphere s2(10);

  Transform3f tf1;
  Transform3f tf2;

  Transform3f transform;
  generateRandomTransform(extents, transform);

  // Vec3f point;
  // FCL_REAL depth;
  Vec3f normal;

  tf1 = Transform3f();
  tf2 = Transform3f(Vec3f(40, 0, 0));
  testShapeInersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3f(Vec3f(40, 0, 0));
  testShapeInersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = Transform3f();
  tf2 = Transform3f(Vec3f(30, 0, 0));
  normal.setValue(1, 0, 0);
  testShapeInersection(s1, tf1, s2, tf2, GST_LIBCCD, true, NULL, NULL, &normal);

  tf1 = Transform3f();
  tf2 = Transform3f(Vec3f(30.01, 0, 0));
  testShapeInersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3f(Vec3f(30.01, 0, 0));
  normal = transform.getRotation() * Vec3f(1, 0, 0);
  testShapeInersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = Transform3f();
  tf2 = Transform3f(Vec3f(29.9, 0, 0));
  normal.setValue(1, 0, 0);
  testShapeInersection(s1, tf1, s2, tf2, GST_LIBCCD, true, NULL, NULL, &normal);

  tf1 = transform;
  tf2 = transform * Transform3f(Vec3f(29.9, 0, 0));
  normal = transform.getRotation() * Vec3f(1, 0, 0);
  testShapeInersection(s1, tf1, s2, tf2, GST_LIBCCD, true, NULL, NULL, &normal);

  tf1 = Transform3f();
  tf2 = Transform3f();
  normal.setZero();  // If the centers of two sphere are at the same position, the normal is (0, 0, 0)
  testShapeInersection(s1, tf1, s2, tf2, GST_LIBCCD, true, NULL, NULL, &normal);

  tf1 = transform;
  tf2 = transform;
  normal.setZero();  // If the centers of two sphere are at the same position, the normal is (0, 0, 0)
  testShapeInersection(s1, tf1, s2, tf2, GST_LIBCCD, true, NULL, NULL, &normal);

  tf1 = Transform3f();
  tf2 = Transform3f(Vec3f(-29.9, 0, 0));
  normal.setValue(-1, 0, 0);
  testShapeInersection(s1, tf1, s2, tf2, GST_LIBCCD, true, NULL, NULL, &normal);

  tf1 = transform;
  tf2 = transform * Transform3f(Vec3f(-29.9, 0, 0));
  normal = transform.getRotation() * Vec3f(-1, 0, 0);
  testShapeInersection(s1, tf1, s2, tf2, GST_LIBCCD, true, NULL, NULL, &normal);

  tf1 = Transform3f();
  tf2 = Transform3f(Vec3f(-30.0, 0, 0));
  normal.setValue(-1, 0, 0);
  testShapeInersection(s1, tf1, s2, tf2, GST_LIBCCD, true, NULL, NULL, &normal);

  tf1 = Transform3f();
  tf2 = Transform3f(Vec3f(-30.01, 0, 0));
  normal.setValue(-1, 0, 0);
  testShapeInersection(s1, tf1, s2, tf2, GST_LIBCCD, false);

  tf1 = transform;
  tf2 = transform * Transform3f(Vec3f(-30.01, 0, 0));
  testShapeInersection(s1, tf1, s2, tf2, GST_LIBCCD, false);
}

BOOST_AUTO_TEST_CASE(shapeIntersection_boxbox)
{
  Box s1(20, 40, 50);
  Box s2(10, 10, 10);

  Transform3f transform;
  generateRandomTransform(extents, transform);
  Transform3f identity;

  CollisionRequest request;
  CollisionResult result;

  bool res;

  res = solver1.shapeIntersect(s1, Transform3f(), s2, Transform3f(), NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(), request, result) > 0);
  BOOST_CHECK(res);

  res = solver1.shapeIntersect(s1, transform, s2, transform, NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform, request, result) > 0);
  BOOST_CHECK(res);

  res = solver1.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(15, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(Vec3f(15, 0, 0)), request, result) > 0);
  BOOST_CHECK(res);

  res = solver1.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(15.01, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK_FALSE(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform * Transform3f(Vec3f(15.01, 0, 0)), request, result) > 0);
  BOOST_CHECK_FALSE(res);

  Quaternion3f q;
  q.fromAxisAngle(Vec3f(0, 0, 1), (FCL_REAL)3.140 / 6);
  res = solver1.shapeIntersect(s1, Transform3f(), s2, Transform3f(q), NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(q), request, result) > 0);
  BOOST_CHECK(res);

  res = solver1.shapeIntersect(s1, transform, s2, transform * Transform3f(q), NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform * Transform3f(q), request, result) > 0);
  BOOST_CHECK(res);
}

BOOST_AUTO_TEST_CASE(shapeIntersection_spherebox)
{
  Sphere s1(20);
  Box s2(5, 5, 5);

  Transform3f transform;
  generateRandomTransform(extents, transform);
  Transform3f identity;

  CollisionRequest request;
  CollisionResult result;

  bool res;

  res = solver1.shapeIntersect(s1, Transform3f(), s2, Transform3f(), NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(), request, result) > 0);
  BOOST_CHECK(res);

  res = solver1.shapeIntersect(s1, transform, s2, transform, NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform, request, result) > 0);
  BOOST_CHECK(res);


  res = solver1.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(22.5, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK_FALSE(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(Vec3f(22.5, 0, 0)), request, result) > 0);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(22.501, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK_FALSE(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform * Transform3f(Vec3f(22.501, 0, 0)), request, result) > 0);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(22.4, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(Vec3f(22.4, 0, 0)), request, result) > 0);
  BOOST_CHECK(res);

  res = solver1.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(22.4, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform * Transform3f(Vec3f(22.4, 0, 0)), request, result) > 0);
  BOOST_CHECK(res);
}

BOOST_AUTO_TEST_CASE(shapeIntersection_cylindercylinder)
{
  Cylinder s1(5, 10);
  Cylinder s2(5, 10);

  Transform3f transform;
  generateRandomTransform(extents, transform);
  Transform3f identity;

  CollisionRequest request;
  CollisionResult result;

  bool res;

  res = solver1.shapeIntersect(s1, Transform3f(), s2, Transform3f(), NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(), request, result) > 0);
  BOOST_CHECK(res);

  res = solver1.shapeIntersect(s1, transform, s2, transform, NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform, request, result) > 0);
  BOOST_CHECK(res);

  res = solver1.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(9.9, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(Vec3f(9.9, 0, 0)), request, result) > 0);
  BOOST_CHECK(res);

  res = solver1.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(9.9, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform * Transform3f(Vec3f(9.9, 0, 0)), request, result) > 0);
  BOOST_CHECK(res);

  res = solver1.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(10, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK_FALSE(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(Vec3f(10, 0, 0)), request, result) > 0);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(10.01, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK_FALSE(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform * Transform3f(Vec3f(10.01, 0, 0)), request, result) > 0);
  BOOST_CHECK_FALSE(res);
}

BOOST_AUTO_TEST_CASE(shapeIntersection_conecone)
{
  Cone s1(5, 10);
  Cone s2(5, 10);

  Transform3f transform;
  generateRandomTransform(extents, transform);
  Transform3f identity;

  CollisionRequest request;
  CollisionResult result;

  bool res;

  res = solver1.shapeIntersect(s1, Transform3f(), s2, Transform3f(), NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(), request, result) > 0);
  BOOST_CHECK(res);

  res = solver1.shapeIntersect(s1, transform, s2, transform, NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform, request, result) > 0);
  BOOST_CHECK(res);

  res = solver1.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(9.9, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(Vec3f(9.9, 0, 0)), request, result) > 0);
  BOOST_CHECK(res);

  res = solver1.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(9.9, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform * Transform3f(Vec3f(9.9, 0, 0)), request, result) > 0);
  BOOST_CHECK(res);

  res = solver1.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(10.001, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK_FALSE(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(Vec3f(10.001, 0, 0)), request, result) > 0);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(10.001, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK_FALSE(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform * Transform3f(Vec3f(10.001, 0, 0)), request, result) > 0);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(0, 0, 9.9)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(Vec3f(0, 0, 9.9)), request, result) > 0);
  BOOST_CHECK(res);

  res = solver1.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(0, 0, 9.9)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform * Transform3f(Vec3f(0, 0, 9.9)), request, result) > 0);
  BOOST_CHECK(res);
}

BOOST_AUTO_TEST_CASE(shapeIntersection_conecylinder)
{
  Cylinder s1(5, 10);
  Cone s2(5, 10);

  Transform3f transform;
  generateRandomTransform(extents, transform);
  Transform3f identity;

  CollisionRequest request;
  CollisionResult result;

  bool res;

  res = solver1.shapeIntersect(s1, Transform3f(), s2, Transform3f(), NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(), request, result) > 0);
  BOOST_CHECK(res);

  res = solver1.shapeIntersect(s1, transform, s2, transform, NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform, request, result) > 0);
  BOOST_CHECK(res);

  res = solver1.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(9.9, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(Vec3f(9.9, 0, 0)), request, result) > 0);
  BOOST_CHECK(res);

  res = solver1.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(9.9, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform * Transform3f(Vec3f(9.9, 0, 0)), request, result) > 0);
  BOOST_CHECK(res);

  res = solver1.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(10.01, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK_FALSE(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(Vec3f(10, 0, 0)), request, result) > 0);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(10.01, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK_FALSE(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform * Transform3f(Vec3f(10.01, 0, 0)), request, result) > 0);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(0, 0, 9.9)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(Vec3f(0, 0, 9.9)), request, result) > 0);
  BOOST_CHECK(res);

  res = solver1.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(0, 0, 9.9)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform * Transform3f(Vec3f(0, 0, 9.9)), request, result) > 0);
  BOOST_CHECK(res);

  res = solver1.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(0, 0, 10.01)), NULL, NULL, NULL);
  BOOST_CHECK_FALSE(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(Vec3f(0, 0, 10)), request, result) > 0);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(0, 0, 10.01)), NULL, NULL, NULL);
  BOOST_CHECK_FALSE(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform * Transform3f(Vec3f(0, 0, 10.01)), request, result) > 0);
  BOOST_CHECK_FALSE(res);
}

BOOST_AUTO_TEST_CASE(shapeIntersection_spheretriangle)
{
  Sphere s(10);
  Vec3f t[3];
  t[0].setValue(20, 0, 0);
  t[1].setValue(-20, 0, 0);
  t[2].setValue(0, 20, 0);

  Transform3f transform;
  generateRandomTransform(extents, transform);
  Transform3f identity;

  bool res;

  res = solver1.shapeTriangleIntersect(s, Transform3f(), t[0], t[1], t[2], NULL, NULL, NULL);
  BOOST_CHECK(res);

  res =  solver1.shapeTriangleIntersect(s, transform, t[0], t[1], t[2], transform, NULL, NULL, NULL);
  BOOST_CHECK(res);


  t[0].setValue(30, 0, 0);
  t[1].setValue(9.9, -20, 0);
  t[2].setValue(9.9, 20, 0);
  res = solver1.shapeTriangleIntersect(s, Transform3f(), t[0], t[1], t[2], NULL, NULL, NULL);
  BOOST_CHECK(res);

  res =  solver1.shapeTriangleIntersect(s, transform, t[0], t[1], t[2], transform, NULL, NULL, NULL);
  BOOST_CHECK(res);
}

BOOST_AUTO_TEST_CASE(shapeIntersection_halfspacesphere)
{
  Sphere s(10);
  Halfspace hs(Vec3f(1, 0, 0), 0);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  Vec3f contact;
  FCL_REAL depth;
  Vec3f normal;
  bool res;

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(-5, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform, &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(-5, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 15) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(-2.5, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 15) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(-2.5, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(-5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(-7.5, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(-5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(-7.5, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(-10.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(-10.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(10.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 20.1) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0.05, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(10.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 20.1) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0.05, 0, 0))));
}

BOOST_AUTO_TEST_CASE(shapeIntersection_planesphere)
{
  Sphere s(10);
  Plane hs(Vec3f(1, 0, 0), 0);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  Vec3f contact;
  FCL_REAL depth;
  Vec3f normal;
  bool res;

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)) || normal.equal(Vec3f(1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform, &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))) || normal.equal(transform.getQuatRotation().transform(Vec3f(1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(5, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(5, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(-5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(-5, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(-5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(-5, 0, 0))));


  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(-10.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(-10.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(10.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(10.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);
}

BOOST_AUTO_TEST_CASE(shapeIntersection_halfspacebox)
{
  Box s(5, 10, 20);
  Halfspace hs(Vec3f(1, 0, 0), 0);
  
  Transform3f transform;
  generateRandomTransform(extents, transform);

  Vec3f contact;
  FCL_REAL depth;
  Vec3f normal;
  bool res;

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(-1.25, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform, &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(-1.25, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(1.25, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 3.75) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(-0.625, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(1.25, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 3.75) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(-0.625, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(-1.25, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 1.25) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(-1.875, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(-1.25, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 1.25) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(-1.875, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(2.51, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5.01) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0.005, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(2.51, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5.01) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0.005, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(-2.51, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(-2.51, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, Transform3f(transform.getQuatRotation()), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
}

BOOST_AUTO_TEST_CASE(shapeIntersection_planebox)
{
  Box s(5, 10, 20);
  Plane hs(Vec3f(1, 0, 0), 0);
  
  Transform3f transform;
  generateRandomTransform(extents, transform);

  Vec3f contact;
  FCL_REAL depth;
  Vec3f normal;
  bool res;

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)) || normal.equal(Vec3f(1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform, &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))) || normal.equal(transform.getQuatRotation().transform(Vec3f(1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(1.25, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 1.25) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(1.25, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(1.25, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 1.25) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(1.25, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(-1.25, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 1.25) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(-1.25, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(-1.25, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 1.25) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(-1.25, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(2.51, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(2.51, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(-2.51, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(-2.51, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, Transform3f(transform.getQuatRotation()), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
}

BOOST_AUTO_TEST_CASE(shapeIntersection_halfspacecapsule)
{
  Capsule s(5, 10);
  Halfspace hs(Vec3f(1, 0, 0), 0);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  Vec3f contact;
  FCL_REAL depth;
  Vec3f normal;
  bool res;

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(-2.5, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform, &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(-2.5, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 7.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(-1.25, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 7.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(-1.25, 0, 0))));
  
  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(-2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(-3.75, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(-2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(-3.75, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10.1) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0.05, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10.1) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0.05, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(-5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(-5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);




  hs = Halfspace(Vec3f(0, 1, 0), 0);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, -1, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, -2.5, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform, &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, -1, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, -2.5, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 7.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, -1, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, -1.25, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 7.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, -1, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, -1.25, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, -2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, -1, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, -3.75, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, -2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, -1, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, -3.75, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10.1) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, -1, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0.05, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10.1) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, -1, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0.05, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, -5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, -5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);




  hs = Halfspace(Vec3f(0, 0, 1), 0);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 0, -1)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, -5)));

  res = solver1.shapeIntersect(s, transform, hs, transform, &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, -1))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, -5))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, 2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 12.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 0, -1)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, -3.75)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, 2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 12.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, -1))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, -3.75))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, -2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 7.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 0, -1)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, -6.25)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, -2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 7.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, -1))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, -6.25))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, 10.1)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 20.1) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 0, -1)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, 0.05)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, 10.1)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 20.1) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, -1))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, 0.05))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, -10.1)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, -10.1)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);
}

BOOST_AUTO_TEST_CASE(shapeIntersection_planecapsule)
{
  Capsule s(5, 10);
  Plane hs(Vec3f(1, 0, 0), 0);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  Vec3f contact;
  FCL_REAL depth;
  Vec3f normal;
  bool res;

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)) || normal.equal(Vec3f(1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform, &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))) || normal.equal(transform.getQuatRotation().transform(Vec3f(1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(2.5, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(2.5, 0, 0))));
  
  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(-2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(-2.5, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(-2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(-2.5, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(-5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(-5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);




  hs = Plane(Vec3f(0, 1, 0), 0);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, -1, 0)) || normal.equal(Vec3f(0, 1, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform, &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, -1, 0))) || normal.equal(transform.getQuatRotation().transform(Vec3f(0, 1, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 1, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, 2.5, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 1, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 2.5, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, -2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, -1, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, -2.5, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, -2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, -1, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, -2.5, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, -5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, -5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);




  hs = Plane(Vec3f(0, 0, 1), 0);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 0, -1)) || normal.equal(Vec3f(0, 0, 1)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform, &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, -1))) || normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, 1))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, 2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 7.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 0, 1)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, 2.5)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, 2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 7.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, 1))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, 2.5))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, -2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 7.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 0, -1)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, -2.5)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, -2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 7.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, -1))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, -2.5))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, 10.1)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, 10.1)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, -10.1)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, -10.1)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);
}

BOOST_AUTO_TEST_CASE(shapeIntersection_halfspacecylinder)
{
  Cylinder s(5, 10);
  Halfspace hs(Vec3f(1, 0, 0), 0);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  Vec3f contact;
  FCL_REAL depth;
  Vec3f normal;
  bool res;
  
  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(-2.5, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform, &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(-2.5, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 7.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(-1.25, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 7.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(-1.25, 0, 0))));
  
  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(-2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(-3.75, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(-2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(-3.75, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10.1) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0.05, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10.1) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0.05, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(-5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(-5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);




  hs = Halfspace(Vec3f(0, 1, 0), 0);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, -1, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, -2.5, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform, &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, -1, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, -2.5, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 7.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, -1, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, -1.25, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 7.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, -1, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, -1.25, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, -2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, -1, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, -3.75, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, -2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, -1, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, -3.75, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10.1) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, -1, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0.05, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10.1) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, -1, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0.05, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, -5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, -5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);




  hs = Halfspace(Vec3f(0, 0, 1), 0);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 0, -1)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, -2.5)));

  res = solver1.shapeIntersect(s, transform, hs, transform, &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, -1))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, -2.5))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, 2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 7.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 0, -1)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, -1.25)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, 2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 7.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, -1))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, -1.25))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, -2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 0, -1)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, -3.75)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, -2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, -1))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, -3.75))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, 5.1)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10.1) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 0, -1)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, 0.05)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, 5.1)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10.1) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, -1))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, 0.05))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, -5.1)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, -5.1)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);
}

BOOST_AUTO_TEST_CASE(shapeIntersection_planecylinder)
{
  Cylinder s(5, 10);
  Plane hs(Vec3f(1, 0, 0), 0);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  Vec3f contact;
  FCL_REAL depth;
  Vec3f normal;
  bool res;

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)) || normal.equal(Vec3f(1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform, &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))) || normal.equal(transform.getQuatRotation().transform(Vec3f(1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(2.5, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(2.5, 0, 0))));
  
  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(-2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(-2.5, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(-2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(-2.5, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(-5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(-5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);




  hs = Plane(Vec3f(0, 1, 0), 0);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, -1, 0)) || normal.equal(Vec3f(0, 1, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform, &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, -1, 0))) || normal.equal(transform.getQuatRotation().transform(Vec3f(0, 1, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 1, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, 2.5, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 1, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 2.5, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, -2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, -1, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, -2.5, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, -2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, -1, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, -2.5, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, -5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, -5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);




  hs = Plane(Vec3f(0, 0, 1), 0);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 0, -1)) || normal.equal(Vec3f(0, 0, 1)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform, &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, -1))) || normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, 1))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, 2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 0, 1)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, 2.5)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, 2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, 1))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, 2.5))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, -2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 0, -1)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, -2.5)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, -2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, -1))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, -2.5))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, 10.1)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, 10.1)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, -10.1)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, -10.1)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);
}


BOOST_AUTO_TEST_CASE(shapeIntersection_halfspacecone)
{
  Cone s(5, 10);
  Halfspace hs(Vec3f(1, 0, 0), 0);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  Vec3f contact;
  FCL_REAL depth;
  Vec3f normal;
  bool res;
  
  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(-2.5, 0, -5)));

  res = solver1.shapeIntersect(s, transform, hs, transform, &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(-2.5, 0, -5))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 7.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(-1.25, 0, -5)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 7.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(-1.25, 0, -5))));
  
  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(-2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(-3.75, 0, -5)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(-2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(-3.75, 0, -5))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10.1) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0.05, 0, -5)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10.1) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0.05, 0, -5))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(-5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(-5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);




  hs = Halfspace(Vec3f(0, 1, 0), 0);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, -1, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, -2.5, -5)));

  res = solver1.shapeIntersect(s, transform, hs, transform, &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, -1, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, -2.5, -5))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 7.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, -1, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, -1.25, -5)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 7.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, -1, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, -1.25, -5))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, -2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, -1, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, -3.75, -5)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, -2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, -1, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, -3.75, -5))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10.1) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, -1, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0.05, -5)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10.1) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, -1, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0.05, -5))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, -5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, -5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);




  hs = Halfspace(Vec3f(0, 0, 1), 0);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 0, -1)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, -2.5)));

  res = solver1.shapeIntersect(s, transform, hs, transform, &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, -1))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, -2.5))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, 2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 7.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 0, -1)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, -1.25)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, 2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 7.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, -1))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, -1.25))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, -2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 0, -1)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, -3.75)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, -2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, -1))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, -3.75))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, 5.1)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10.1) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 0, -1)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, 0.05)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, 5.1)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 10.1) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, -1))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, 0.05))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, -5.1)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, -5.1)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);
}

BOOST_AUTO_TEST_CASE(shapeIntersection_planecone)
{
  Cone s(5, 10);
  Plane hs(Vec3f(1, 0, 0), 0);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  Vec3f contact;
  FCL_REAL depth;
  Vec3f normal;
  bool res;

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)) || normal.equal(Vec3f(1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform, &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))) || normal.equal(transform.getQuatRotation().transform(Vec3f(1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(2.5, 0, -2.5)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(2.5, 0, -2.5))));
  
  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(-2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(-1, 0, 0)));
  BOOST_CHECK(contact.equal(Vec3f(-2.5, 0, -2.5)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(-2.5, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(-1, 0, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(-2.5, 0, -2.5))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(-5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(-5.1, 0, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);




  hs = Plane(Vec3f(0, 1, 0), 0);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, -1, 0)) || normal.equal(Vec3f(0, 1, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform, &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, -1, 0))) || normal.equal(transform.getQuatRotation().transform(Vec3f(0, 1, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 1, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, 2.5, -2.5)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 1, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 2.5, -2.5))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, -2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, -1, 0)));
  BOOST_CHECK(contact.equal(Vec3f(0, -2.5, -2.5)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, -2.5, 0)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, -1, 0))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, -2.5, -2.5))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, -5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, -5.1, 0)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);




  hs = Plane(Vec3f(0, 0, 1), 0);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 0, -1)) || normal.equal(Vec3f(0, 0, 1)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, 0)));

  res = solver1.shapeIntersect(s, transform, hs, transform, &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, -1))) || normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, 1))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, 0))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, 2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 0, 1)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, 2.5)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, 2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, 1))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, 2.5))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, -2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(Vec3f(0, 0, -1)));
  BOOST_CHECK(contact.equal(Vec3f(0, 0, -2.5)));

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, -2.5)), &contact, &depth, &normal);
  BOOST_CHECK(res);
  BOOST_CHECK(std::abs(depth - 2.5) < 0.001);
  BOOST_CHECK(normal.equal(transform.getQuatRotation().transform(Vec3f(0, 0, -1))));
  BOOST_CHECK(contact.equal(transform.transform(Vec3f(0, 0, -2.5))));

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, 10.1)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, 10.1)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, Transform3f(), hs, Transform3f(Vec3f(0, 0, -10.1)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeIntersect(s, transform, hs, transform * Transform3f(Vec3f(0, 0, -10.1)), &contact, &depth, &normal);
  BOOST_CHECK_FALSE(res);
}



BOOST_AUTO_TEST_CASE(shapeDistance_spheresphere)
{  
  Sphere s1(20);
  Sphere s2(10);

  Transform3f transform;
  //generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist = -1;
  Vec3f closest_p1, closest_p2;
  res = solver1.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(0, 40, 0)), &dist, &closest_p1, &closest_p2);
  BOOST_CHECK(fabs(dist - 10) < 0.001);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(30.1, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.001);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(29.9, 0, 0)), &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeDistance(s1, Transform3f(Vec3f(40, 0, 0)), s2, Transform3f(), &dist);
  BOOST_CHECK(fabs(dist - 10) < 0.001);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s1, Transform3f(Vec3f(30.1, 0, 0)), s2, Transform3f(), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.001);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s1, Transform3f(Vec3f(29.9, 0, 0)), s2, Transform3f(), &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);


  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(40, 0, 0)), &dist);
  // this is one problem: the precise is low sometimes
  BOOST_CHECK(fabs(dist - 10) < 0.1);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(30.1, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.06);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(29.9, 0, 0)), &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeDistance(s1, transform * Transform3f(Vec3f(40, 0, 0)), s2, transform, &dist);
  BOOST_CHECK(fabs(dist - 10) < 0.1);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s1, transform * Transform3f(Vec3f(30.1, 0, 0)), s2, transform, &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.1);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s1, transform * Transform3f(Vec3f(29.9, 0, 0)), s2, transform, &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);
}

BOOST_AUTO_TEST_CASE(shapeDistance_boxbox)
{                      
  Box s1(20, 40, 50);
  Box s2(10, 10, 10);
  Vec3f closest_p1, closest_p2;

  Transform3f transform;
  //generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist;

  res = solver1.shapeDistance(s1, Transform3f(), s2, Transform3f(), &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform, &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeDistance(s2, Transform3f(), s2, Transform3f(Vec3f(10.1, 0, 0)), &dist, &closest_p1, &closest_p2);
  BOOST_CHECK(fabs(dist - 0.1) < 0.001);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s2, Transform3f(), s2, Transform3f(Vec3f(20.1, 0, 0)), &dist, &closest_p1, &closest_p2);
  BOOST_CHECK(fabs(dist - 10.1) < 0.001);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s2, Transform3f(), s2, Transform3f(Vec3f(0, 20.2, 0)), &dist, &closest_p1, &closest_p2);
  BOOST_CHECK(fabs(dist - 10.2) < 0.001);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s2, Transform3f(), s2, Transform3f(Vec3f(10.1, 10.1, 0)), &dist, &closest_p1, &closest_p2);
  BOOST_CHECK(fabs(dist - 0.1 * 1.414) < 0.001);
  BOOST_CHECK(res);

  res = solver2.shapeDistance(s2, Transform3f(), s2, Transform3f(Vec3f(10.1, 0, 0)), &dist, &closest_p1, &closest_p2);
  BOOST_CHECK(fabs(dist - 0.1) < 0.001);
  BOOST_CHECK(res);

  res = solver2.shapeDistance(s2, Transform3f(), s2, Transform3f(Vec3f(20.1, 0, 0)), &dist, &closest_p1, &closest_p2);
  BOOST_CHECK(fabs(dist - 10.1) < 0.001);
  BOOST_CHECK(res);

  res = solver2.shapeDistance(s2, Transform3f(), s2, Transform3f(Vec3f(0, 20.1, 0)), &dist, &closest_p1, &closest_p2);
  BOOST_CHECK(fabs(dist - 10.1) < 0.001);
  BOOST_CHECK(res);

  res = solver2.shapeDistance(s2, Transform3f(), s2, Transform3f(Vec3f(10.1, 10.1, 0)), &dist, &closest_p1, &closest_p2);
  BOOST_CHECK(fabs(dist - 0.1 * 1.414) < 0.001);
  BOOST_CHECK(res);


  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(15.1, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.001);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(20, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 5) < 0.001);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(20, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 5) < 0.001);
  BOOST_CHECK(res);
}

BOOST_AUTO_TEST_CASE(shapeDistance_boxsphere)
{
  Sphere s1(20);
  Box s2(5, 5, 5);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist;

  res = solver1.shapeDistance(s1, Transform3f(), s2, Transform3f(), &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform, &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(22.6, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.001);
  BOOST_CHECK(res);
  
  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(22.6, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.05);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(40, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 17.5) < 0.001);
  BOOST_CHECK(res);
  
  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(40, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 17.5) < 0.001);
  BOOST_CHECK(res);
}

BOOST_AUTO_TEST_CASE(shapeDistance_cylindercylinder)
{
  Cylinder s1(5, 10);
  Cylinder s2(5, 10);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist;

  res = solver1.shapeDistance(s1, Transform3f(), s2, Transform3f(), &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform, &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(10.1, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.001);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(10.1, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.001);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(40, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 30) < 0.001);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(40, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 30) < 0.001);
  BOOST_CHECK(res);
}



BOOST_AUTO_TEST_CASE(shapeDistance_conecone)
{
  Cone s1(5, 10);
  Cone s2(5, 10);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist;

  res = solver1.shapeDistance(s1, Transform3f(), s2, Transform3f(), &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform, &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(10.1, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.001);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(10.1, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.001);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(0, 0, 40)), &dist);
  BOOST_CHECK(fabs(dist - 30) < 1);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(0, 0, 40)), &dist);
  BOOST_CHECK(fabs(dist - 30) < 1);
  BOOST_CHECK(res);
}

BOOST_AUTO_TEST_CASE(shapeDistance_conecylinder)
{
  Cylinder s1(5, 10);
  Cone s2(5, 10);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist;

  res = solver1.shapeDistance(s1, Transform3f(), s2, Transform3f(), &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeDistance(s1, transform, s2, transform, &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);

  res = solver1.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(10.1, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.01);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(10.1, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.02);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(40, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 30) < 0.01);
  BOOST_CHECK(res);

  res = solver1.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(40, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 30) < 0.1);
  BOOST_CHECK(res);
}



BOOST_AUTO_TEST_CASE(shapeIntersectionGJK_spheresphere)
{
  Sphere s1(20);
  Sphere s2(10);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  CollisionRequest request;
  CollisionResult result;

  Vec3f contact;
  FCL_REAL penetration_depth;
  Vec3f normal;  
  bool res;

  request.gjk_solver_type = GST_INDEP; // use indep GJK solver

  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(40, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK_FALSE(res); 
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(40, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK_FALSE(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(Vec3f(40, 0, 0)), request, result) > 0);
  BOOST_CHECK_FALSE(res);

  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(40, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK_FALSE(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(40, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK_FALSE(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform * Transform3f(Vec3f(40, 0, 0)), request, result) > 0);
  BOOST_CHECK_FALSE(res);


  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(30, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(30, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(Vec3f(30, 0, 0)), request, result) > 0);
  BOOST_CHECK(res);


  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(30.01, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK_FALSE(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(30.01, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK_FALSE(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform * Transform3f(Vec3f(30.01, 0, 0)), request, result) > 0);
  BOOST_CHECK_FALSE(res);


  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(29.9, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(29.9, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(Vec3f(29.9, 0, 0)), request, result) > 0);
  BOOST_CHECK(res);


  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(29.9, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(29.9, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform * Transform3f(Vec3f(29.9, 0, 0)), request, result) > 0);
  BOOST_CHECK(res);


  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(), request, result) > 0);
  BOOST_CHECK(res);


  res = solver2.shapeIntersect(s1, transform, s2, transform, NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform, &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform, request, result) > 0);
  BOOST_CHECK(res);


  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(-29.9, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(-29.9, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(Vec3f(-29.9, 0, 0)), request, result) > 0);
  BOOST_CHECK(res);



  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(-29.9, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(-29.9, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform * Transform3f(Vec3f(-29.9, 0, 0)), request, result) > 0);
  BOOST_CHECK(res);


  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(-30, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(-30, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);
  result.clear();
  res = (collide(&s1, Transform3f(), &s2, Transform3f(Vec3f(-30, 0, 0)), request, result) > 0);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(-30.01, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK_FALSE(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(-30.01, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK_FALSE(res);
  result.clear();
  res = (collide(&s1, transform, &s2, transform * Transform3f(Vec3f(-30.01, 0, 0)), request, result) > 0);
  BOOST_CHECK_FALSE(res);
}

BOOST_AUTO_TEST_CASE(shapeIntersectionGJK_boxbox)
{
  Box s1(20, 40, 50);
  Box s2(10, 10, 10);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  Vec3f contact;
  FCL_REAL penetration_depth;
  Vec3f normal;  
  bool res;

  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, transform, s2, transform, NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform, &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(15, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(15, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(15.01, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK_FALSE(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(15.01, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK_FALSE(res);

  Quaternion3f q;
  q.fromAxisAngle(Vec3f(0, 0, 1), (FCL_REAL)3.140 / 6);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(q), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(q), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);
  
  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(q), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(q), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);
}

BOOST_AUTO_TEST_CASE(shapeIntersectionGJK_spherebox)
{
  Sphere s1(20);
  Box s2(5, 5, 5);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  Vec3f contact;
  FCL_REAL penetration_depth;
  Vec3f normal;  
  bool res;

  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, transform, s2, transform, NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform, &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(22.5, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(22.5, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(22.51, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK_FALSE(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(22.51, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(22.4, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(22.4, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(22.4, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(22.4, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);
}

BOOST_AUTO_TEST_CASE(shapeIntersectionGJK_cylindercylinder)
{
  Cylinder s1(5, 10);
  Cylinder s2(5, 10);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  Vec3f contact;
  FCL_REAL penetration_depth;
  Vec3f normal;  
  bool res;

  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, transform, s2, transform, NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform, &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(9.9, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(9.9, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(9.9, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(9.9, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(10, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(10, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(10.1, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK_FALSE(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(10.1, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK_FALSE(res);
}

BOOST_AUTO_TEST_CASE(shapeIntersectionGJK_conecone)
{
  Cone s1(5, 10);
  Cone s2(5, 10);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  Vec3f contact;
  FCL_REAL penetration_depth;
  Vec3f normal;  
  bool res;

  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, transform, s2, transform, NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform, &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(9.9, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(9.9, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(9.9, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(9.9, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(10.1, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK_FALSE(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(10.1, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(10.1, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK_FALSE(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(10.1, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK_FALSE(res);

  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(0, 0, 9.9)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(0, 0, 9.9)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(0, 0, 9.9)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(0, 0, 9.9)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);
}

BOOST_AUTO_TEST_CASE(shapeIntersectionGJK_conecylinder)
{
  Cylinder s1(5, 10);
  Cone s2(5, 10);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  Vec3f contact;
  FCL_REAL penetration_depth;
  Vec3f normal;  
  bool res;

  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, transform, s2, transform, NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform, &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(9.9, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(9.9, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(9.9, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(9.9, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(10, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(10, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(10, 0, 0)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(10, 0, 0)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(0, 0, 9.9)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(0, 0, 9.9)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(0, 0, 9.9)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(0, 0, 9.9)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(0, 0, 10)), NULL, NULL, NULL);
  BOOST_CHECK(res);
  res = solver2.shapeIntersect(s1, Transform3f(), s2, Transform3f(Vec3f(0, 0, 10)), &contact, &penetration_depth, &normal);
  BOOST_CHECK(res);

  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(0, 0, 10.1)), NULL, NULL, NULL);
  BOOST_CHECK_FALSE(res);
  res = solver2.shapeIntersect(s1, transform, s2, transform * Transform3f(Vec3f(0, 0, 10.1)), &contact, &penetration_depth, &normal);
  BOOST_CHECK_FALSE(res);
}


BOOST_AUTO_TEST_CASE(shapeIntersectionGJK_spheretriangle)
{
  Sphere s(10);
  Vec3f t[3];
  t[0].setValue(20, 0, 0);
  t[1].setValue(-20, 0, 0);
  t[2].setValue(0, 20, 0);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  bool res;

  res = solver2.shapeTriangleIntersect(s, Transform3f(), t[0], t[1], t[2], NULL, NULL, NULL);
  BOOST_CHECK(res);

  res =  solver2.shapeTriangleIntersect(s, transform, t[0], t[1], t[2], transform, NULL, NULL, NULL);
  BOOST_CHECK(res);

  t[0].setValue(30, 0, 0);
  t[1].setValue(9.9, -20, 0);
  t[2].setValue(9.9, 20, 0);
  res = solver2.shapeTriangleIntersect(s, Transform3f(), t[0], t[1], t[2], NULL, NULL, NULL);
  BOOST_CHECK(res);

  res =  solver2.shapeTriangleIntersect(s, transform, t[0], t[1], t[2], transform, NULL, NULL, NULL);
  BOOST_CHECK(res);
}




BOOST_AUTO_TEST_CASE(spheresphere)
{  
  Sphere s1(20);
  Sphere s2(10);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist = -1;
  
  res = solver2.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(40, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 10) < 0.001);
  BOOST_CHECK(res);

  res = solver2.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(30.1, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.001);
  BOOST_CHECK(res);

  res = solver2.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(29.9, 0, 0)), &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);

  res = solver2.shapeDistance(s1, Transform3f(Vec3f(40, 0, 0)), s2, Transform3f(), &dist);
  BOOST_CHECK(fabs(dist - 10) < 0.001);
  BOOST_CHECK(res);

  res = solver2.shapeDistance(s1, Transform3f(Vec3f(30.1, 0, 0)), s2, Transform3f(), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.001);
  BOOST_CHECK(res);

  res = solver2.shapeDistance(s1, Transform3f(Vec3f(29.9, 0, 0)), s2, Transform3f(), &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);


  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(40, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 10) < 0.001);
  BOOST_CHECK(res);

  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(30.1, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.001);
  BOOST_CHECK(res);

  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(29.9, 0, 0)), &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);

  res = solver2.shapeDistance(s1, transform * Transform3f(Vec3f(40, 0, 0)), s2, transform, &dist);
  BOOST_CHECK(fabs(dist - 10) < 0.001);
  BOOST_CHECK(res);

  res = solver2.shapeDistance(s1, transform * Transform3f(Vec3f(30.1, 0, 0)), s2, transform, &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.001);
  BOOST_CHECK(res);

  res = solver2.shapeDistance(s1, transform * Transform3f(Vec3f(29.9, 0, 0)), s2, transform, &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);
}

BOOST_AUTO_TEST_CASE(boxbox)
{                      
  Box s1(20, 40, 50);
  Box s2(10, 10, 10);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist;

  res = solver2.shapeDistance(s1, Transform3f(), s2, Transform3f(), &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);

  res = solver2.shapeDistance(s1, transform, s2, transform, &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);

  res = solver2.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(15.1, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.001);
  BOOST_CHECK(res);

  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(15.1, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.001);
  BOOST_CHECK(res);

  res = solver2.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(20, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 5) < 0.001);
  BOOST_CHECK(res);

  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(20, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 5) < 0.001);
  BOOST_CHECK(res);
}

BOOST_AUTO_TEST_CASE(boxsphere)
{
  Sphere s1(20);
  Box s2(5, 5, 5);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist;

  res = solver2.shapeDistance(s1, Transform3f(), s2, Transform3f(), &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);

  res = solver2.shapeDistance(s1, transform, s2, transform, &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);

  res = solver2.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(22.6, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.01);
  BOOST_CHECK(res);
  
  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(22.6, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.01);
  BOOST_CHECK(res);

  res = solver2.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(40, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 17.5) < 0.001);
  BOOST_CHECK(res);
  
  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(40, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 17.5) < 0.001);
  BOOST_CHECK(res);
}

BOOST_AUTO_TEST_CASE(cylindercylinder)
{
  Cylinder s1(5, 10);
  Cylinder s2(5, 10);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist;

  res = solver2.shapeDistance(s1, Transform3f(), s2, Transform3f(), &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);

  res = solver2.shapeDistance(s1, transform, s2, transform, &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);

  res = solver2.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(10.1, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.001);
  BOOST_CHECK(res);

  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(10.1, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.001);
  BOOST_CHECK(res);

  res = solver2.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(40, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 30) < 0.001);
  BOOST_CHECK(res);

  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(40, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 30) < 0.001);
  BOOST_CHECK(res);
}



BOOST_AUTO_TEST_CASE(conecone)
{
  Cone s1(5, 10);
  Cone s2(5, 10);

  Transform3f transform;
  generateRandomTransform(extents, transform);

  bool res;
  FCL_REAL dist;

  res = solver2.shapeDistance(s1, Transform3f(), s2, Transform3f(), &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);

  res = solver2.shapeDistance(s1, transform, s2, transform, &dist);
  BOOST_CHECK(dist < 0);
  BOOST_CHECK_FALSE(res);

  res = solver2.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(10.1, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.001);
  BOOST_CHECK(res);

  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(10.1, 0, 0)), &dist);
  BOOST_CHECK(fabs(dist - 0.1) < 0.001);
  BOOST_CHECK(res);

  res = solver2.shapeDistance(s1, Transform3f(), s2, Transform3f(Vec3f(0, 0, 40)), &dist);
  BOOST_CHECK(fabs(dist - 30) < 0.001);
  BOOST_CHECK(res);

  res = solver2.shapeDistance(s1, transform, s2, transform * Transform3f(Vec3f(0, 0, 40)), &dist);
  BOOST_CHECK(fabs(dist - 30) < 0.001);
  BOOST_CHECK(res);
}




