#include <utest/utest.hpp>

#include <boost/optional.hpp>

#include <rcu/rcu.hpp>

using X = std::pair<int, int>;

TEST(Rcu, Ctr) { rcu::Variable<X> ptr; }

TEST(Rcu, ReadInit) {
  rcu::Variable<X> ptr(1, 2);

  auto reader = ptr.Read();
  EXPECT_EQ(std::make_pair(1, 2), *reader);
}

TEST(Rcu, ChangeRead) {
  RunInCoro([] {
    rcu::Variable<X> ptr(1, 2);

    {
      auto writer = ptr.StartWrite();
      writer->first = 3;
      writer.Commit();
    }

    auto reader = ptr.Read();
    EXPECT_EQ(std::make_pair(3, 2), *reader);
  });
}

TEST(Rcu, ChangeCancelRead) {
  RunInCoro([] {
    rcu::Variable<X> ptr(1, 2);

    {
      auto writer = ptr.StartWrite();
      writer->first = 3;
    }

    auto reader = ptr.Read();
    EXPECT_EQ(std::make_pair(1, 2), *reader);
  });
}

TEST(Rcu, AssignRead) {
  RunInCoro([] {
    rcu::Variable<X> ptr(1, 2);

    ptr.Assign({3, 4});

    auto reader = ptr.Read();
    EXPECT_EQ(std::make_pair(3, 4), *reader);
  });
}

TEST(Rcu, ReadNotCommitted) {
  RunInCoro([] {
    rcu::Variable<X> ptr(1, 2);

    auto reader1 = ptr.Read();
    EXPECT_EQ(std::make_pair(1, 2), *reader1);

    {
      auto writer = ptr.StartWrite();
      writer->second = 3;
      EXPECT_EQ(std::make_pair(1, 2), *reader1);

      auto reader2 = ptr.Read();
      EXPECT_EQ(std::make_pair(1, 2), *reader2);
    }

    EXPECT_EQ(std::make_pair(1, 2), *reader1);

    auto reader3 = ptr.Read();
    EXPECT_EQ(std::make_pair(1, 2), *reader3);
  });
}

TEST(Rcu, ReadCommitted) {
  RunInCoro([] {
    rcu::Variable<X> ptr(1, 2);

    auto reader1 = ptr.Read();

    auto writer = ptr.StartWrite();
    writer->second = 3;
    auto reader2 = ptr.Read();

    writer.Commit();
    EXPECT_EQ(std::make_pair(1, 2), *reader1);
    EXPECT_EQ(std::make_pair(1, 2), *reader2);

    auto reader3 = ptr.Read();
    EXPECT_EQ(std::make_pair(1, 3), *reader3);
  });
}

struct Counted {
  Counted() { counter++; }
  Counted(const Counted&) : Counted() {}
  Counted(Counted&&) = delete;

  ~Counted() { counter--; }

  int value{1};

  static size_t counter;
};

size_t Counted::counter = 0;

TEST(Rcu, Lifetime) {
  RunInCoro([] {
    EXPECT_EQ(0, Counted::counter);

    rcu::Variable<Counted> ptr;
    EXPECT_EQ(1, Counted::counter);

    {
      auto reader = ptr.Read();
      EXPECT_EQ(1, Counted::counter);
    }
    EXPECT_EQ(1, Counted::counter);

    {
      auto writer = ptr.StartWrite();
      EXPECT_EQ(2, Counted::counter);
    }
    EXPECT_EQ(1, Counted::counter);

    {
      auto reader2 = ptr.Read();
      EXPECT_EQ(1, Counted::counter);
      {
        auto writer = ptr.StartWrite();
        EXPECT_EQ(2, Counted::counter);

        writer->value = 10;
        writer.Commit();
        EXPECT_EQ(2, Counted::counter);
      }
      EXPECT_EQ(2, Counted::counter);
    }

    EXPECT_EQ(2, Counted::counter);

    {
      auto writer = ptr.StartWrite();
      EXPECT_EQ(3, Counted::counter);

      writer->value = 10;
      writer.Commit();
      EXPECT_EQ(1, Counted::counter);
    }
    EXPECT_EQ(1, Counted::counter);
  });
}

TEST(Rcu, NoCopy) {
  struct X {
    X(X&&) = default;
    X(const X&) = delete;
    X& operator=(const X&) = delete;

    bool operator==(const X& other) const {
      return std::tie(x, y) == std::tie(other.x, other.y);
    }

    int x{};
    bool y{};
  };

  RunInCoro([] {
    rcu::Variable<X> ptr(X{1, false});

    // Write
    ptr.Assign(X{2, true});

    // Won't compile if uncomment, no X::X(const X&)
    // auto write_ptr = ptr.StartWrite();

    auto reader = ptr.Read();
    EXPECT_EQ((X{2, true}), *reader);
  });
}

TEST(Rcu, HpCacheReuse) {
  RunInCoro([] {
    boost::optional<rcu::Variable<int>> vars;
    vars.emplace(42);
    EXPECT_EQ(42, *vars->Read());

    vars.reset();

    // caused UAF because of stale HP cache -- TAXICOMMON-1506
    vars.emplace(666);
    EXPECT_EQ(666, *vars->Read());
  });
}
