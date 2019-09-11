#include <storages/postgres/io/geometry_types.hpp>
#include <storages/postgres/tests/util_test.hpp>

namespace pg = storages::postgres;
namespace io = pg::io;
namespace tt = io::traits;

namespace {

POSTGRE_TEST_P(InternalGeometryPointRoundtrip) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

  pg::ResultSet res{nullptr};
  io::detail::Point p;
  EXPECT_NO_THROW(res = conn->Execute("select $1", io::detail::Point{1, 2}));
  EXPECT_NO_THROW(res[0][0].To(p));
  EXPECT_EQ((io::detail::Point{1, 2}), p);
}
//
POSTGRE_TEST_P(InternalGeometryLineRoundtrip) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

  pg::ResultSet res{nullptr};
  io::detail::Line l;
  EXPECT_NO_THROW(res = conn->Execute("select $1", io::detail::Line{1, 2, 3}));
  EXPECT_NO_THROW(res[0][0].To(l));
  EXPECT_EQ((io::detail::Line{1, 2, 3}), l);
}

POSTGRE_TEST_P(InternalGeometryLsegRoundtrip) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

  pg::ResultSet res{nullptr};
  io::detail::LineSegment l;
  EXPECT_NO_THROW(res = conn->Execute("select $1", io::detail::LineSegment{
                                                       {{{-1, 1}, {1, -1}}}}));
  EXPECT_NO_THROW(res[0][0].To(l));
  EXPECT_EQ((io::detail::LineSegment{{{{-1, 1}, {1, -1}}}}), l);
}

POSTGRE_TEST_P(InternalGeometryBoxRoundtrip) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

  pg::ResultSet res{nullptr};
  io::detail::Box b;
  EXPECT_NO_THROW(
      res = conn->Execute("select $1", io::detail::Box{{{{1, 1}, {0, 0}}}}));
  EXPECT_NO_THROW(res[0][0].To(b));
  EXPECT_EQ((io::detail::Box{{{{1, 1}, {0, 0}}}}), b);
}

POSTGRE_TEST_P(InternalGeometryPathRoundtrip) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

  pg::ResultSet res{nullptr};
  io::detail::Path p;
  EXPECT_NO_THROW(
      res = conn->Execute("select $1",
                          io::detail::Path{true, {{1, 1}, {0, 0}, {-1, 1}}}));
  EXPECT_NO_THROW(res[0][0].To(p));
  EXPECT_EQ((io::detail::Path{true, {{1, 1}, {0, 0}, {-1, 1}}}), p);
  EXPECT_NO_THROW(
      res = conn->Execute("select $1",
                          io::detail::Path{false, {{1, 1}, {0, 0}, {-1, 1}}}));
  EXPECT_NO_THROW(res[0][0].To(p));
  EXPECT_EQ((io::detail::Path{false, {{1, 1}, {0, 0}, {-1, 1}}}), p);
}

POSTGRE_TEST_P(InternalGeometryPolygonRoundtrip) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

  pg::ResultSet res{nullptr};
  io::detail::Polygon p;
  EXPECT_NO_THROW(
      res = conn->Execute("select $1",
                          io::detail::Polygon{{{1, 1}, {0, 0}, {-1, 1}}}));
  EXPECT_NO_THROW(res[0][0].To(p));
  EXPECT_EQ((io::detail::Polygon{{{1, 1}, {0, 0}, {-1, 1}}}), p);
}

POSTGRE_TEST_P(InternalGeometryCircleRoundtrip) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

  pg::ResultSet res{nullptr};
  io::detail::Circle c;
  EXPECT_NO_THROW(
      res = conn->Execute("select $1", io::detail::Circle{{1, 2}, 3}));
  EXPECT_NO_THROW(res[0][0].To(c));
  EXPECT_EQ((io::detail::Circle{{1, 2}, 3}), c);
}

}  // namespace