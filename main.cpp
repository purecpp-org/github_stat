#include <cinatra/coro_http_client.hpp>
#include <dbng.hpp>
#include <iguana/json.hpp>
#include <iostream>
#include <sqlite.hpp>

struct person {
  int id;
  std::string name;
  int age;
};
REFLECTION(person, id, name, age)

void test_db() {
  person p{};
  constexpr auto r = __cplusplus;
  std::string str;

  iguana::json::from_json(p, str.data());
  ormpp::dbng<ormpp::sqlite> sqlite;
  const char *db = "test_ormppdb";
  sqlite.connect(db);
  sqlite.create_datatable<person>(ormpp_auto_key{"id"});

  {
    sqlite.delete_records<person>();
    sqlite.insert<person>({0, "purecpp"});
    sqlite.insert<person>({0, "purecpp", 6});
    auto vec = sqlite.query<person>();
    for (auto &[id, name, age] : vec) {
      std::cout << id << ", " << name << ", " << age << "\n";
    }
  }
}

int main() {
  test_db();
  auto &pool = coro_io::g_io_context_pool(2);
  cinatra::coro_http_client client(pool.get_executor());
  std::cout << "Hello, World!" << std::endl;
  return 0;
}
