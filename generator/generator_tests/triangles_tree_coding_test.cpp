#include "generator/tesselator.hpp"

#include "indexer/geometry_serialization.hpp"
#include "indexer/mercator.hpp"
#include "indexer/point_to_int64.hpp"
#include "indexer/coding_params.hpp"

#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "testing/testing.hpp"


namespace
{
  typedef m2::PointD P;

  bool is_equal(P const & p1, P const & p2)
  {
    return p1.EqualDxDy(p2, MercatorBounds::GetCellID2PointAbsEpsilon());
  }

  bool FindTriangle(serial::OutPointsT const & test, P arr[])
  {
    size_t const count = test.size();
    for (size_t i = 0; i < count; i+=3)
    {
      for (int base = 0; base < 3; ++base)
        if (is_equal(test[i], arr[base]) &&
            is_equal(test[i+1], arr[(base+1)%3]) &&
            is_equal(test[i+2], arr[(base+2)%3]))
        {
          return true;
        }
    }
    return false;
  }

  void CompareTriangles(serial::OutPointsT const & test,
                        P arrP[], int arrT[][3], size_t count)
  {
    TEST_EQUAL(test.size(), 3*count, (test));

    for (size_t i = 0; i < count; ++i)
    {
      P trg[] = { arrP[arrT[i][0]], arrP[arrT[i][1]], arrP[arrT[i][2]] };
      TEST ( FindTriangle(test, trg), ("Triangles : ", test, " Etalon : ", trg[0], trg[1], trg[2]) );
    }
  }

  void TestTrianglesCoding(P arrP[], size_t countP, int arrT[][3], size_t countT)
  {
    tesselator::TrianglesInfo info;
    info.AssignPoints(arrP, arrP + countP);

    info.Reserve(countT);

    for (size_t i = 0; i < countT; ++i)
      info.Add(arrT[i][0], arrT[i][1], arrT[i][2]);

    serial::CodingParams cp;

    serial::TrianglesChainSaver saver(cp);
    tesselator::PointsInfo points;
    m2::PointU (* D2U)(m2::PointD const &, uint32_t) = &PointD2PointU;
    info.GetPointsInfo(saver.GetBasePoint(), saver.GetMaxPoint(),
                       bind(D2U, _1, cp.GetCoordBits()), points);

    info.ProcessPortions(points, saver);

    vector<char> buffer;
    MemWriter<vector<char> > writer(buffer);
    saver.Save(writer);

    TEST ( !buffer.empty(), () );

    MemReader reader(&buffer[0], buffer.size());
    ReaderSource<MemReader> src(reader);

    serial::OutPointsT triangles;
    serial::LoadOuterTriangles(src, cp, triangles);

    CompareTriangles(triangles, arrP, arrT, countT);
  }
}

UNIT_TEST(TrianglesCoding_Smoke)
{
  {
    P arrP[] =  { P(0, 0), P(0, 1), P(1, 0), P(1, 1), P(0, -1), P(-1, 0) };
    int arrT[][3] = { {0, 1, 2}, {1, 3, 2}, {4, 0, 2}, {1, 0, 5}, {4, 5, 0} };

    TestTrianglesCoding(arrP, ARRAY_SIZE(arrP), arrT, ARRAY_SIZE(arrT));
  }
}

UNIT_TEST(TrianglesCoding_Rect)
{
  {
    P arrP[] =  { P(-16.874999848078005, -44.999999874271452),
                  P(-16.874999848078005, -39.374999869032763),
                  P(-11.249999842839316, -39.374999869032763),
                  P(-11.249999842839316, -44.999999874271452)
                };

    int arrT[][3] = { {2, 0, 1}, {0, 2, 3} };

    TestTrianglesCoding(arrP, ARRAY_SIZE(arrP), arrT, ARRAY_SIZE(arrT));
  }
}
