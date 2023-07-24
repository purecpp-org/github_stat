#include "cinatra/define.h"
#include <charconv>
#include <chrono>
#include <cinatra.hpp>
#include <cinatra/time_util.hpp>
#include <cstdint>
#include <dbng.hpp>
#include <iguana/json_reader.hpp>
#include <iostream>
#include <sqlite.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

struct clone_t {
  std::string timestamp;
  int64_t count;
  int64_t uniques;
  int64_t unix_time;
};
REFLECTION(clone_t, timestamp, count, uniques, unix_time);

struct github_clones {
  int64_t count;
  int64_t uniques;
  std::vector<clone_t> clones;
};
REFLECTION(github_clones, count, uniques, clones);

inline int64_t get_time_stamp(std::string_view str) {
  // must be such format: "2023-07-10"
  if (str.size() != 10) {
    throw std::invalid_argument("time str length is not 10!");
  }
  int year{}, month{}, day{};

  int index = 0;
  if (auto [p, ec] = std::from_chars(str.data(), str.data() + 4, year);
      ec != std::errc{}) {
    throw std::invalid_argument("parse int error: " +
                                std::make_error_code(ec).message());
  }
  index += 5;
  if (auto [p, ec] =
          std::from_chars(str.data() + index, str.data() + index + 2, month);
      ec != std::errc{}) {
    throw std::invalid_argument("parse int error: " +
                                std::make_error_code(ec).message());
  }

  month -= 1;
  index += 3;

  if (auto [p, ec] =
          std::from_chars(str.data() + index, str.data() + index + 2, day);
      ec != std::errc{}) {
    throw std::invalid_argument("parse int error: " +
                                std::make_error_code(ec).message());
  }

  auto [ok, tm] =
      cinatra::time_util::faster_mktime(year, month, day, 0, 0, 0, -1);
  if (!ok) {
    throw std::invalid_argument("faster_mktime error: ");
  }

  return tm;
}

inline std::string read_table() {
  std::string result;
  std::vector<std::string> names{"yalantinglibs", "async_simple"};
  for (auto &name : names) {
    ormpp::dbng<ormpp::sqlite> sqlite;
    sqlite.connect((name + ".db").data());
    sqlite.create_datatable<clone_t>(ormpp_auto_key{"unix_time"});
    auto tp = sqlite.query<std::tuple<int>>("select sum(uniques) from clone_t");

    result.append(name)
        .append(" unique clones: ")
        .append(std::to_string(std::get<0>(tp[0])))
        .append(", details:\n");

    auto vec = sqlite.query<clone_t>();
    for (auto &item : vec) {
      result.append(item.timestamp)
          .append(", ")
          .append(std::to_string(item.count))
          .append(", ")
          .append(std::to_string(item.uniques))
          .append("\n");
    }
    result.append("\n");
  }

  return result;
}

void update_table(const std::vector<std::string> &args,
                  const std::string &auth_token, auto &pool) {
  for (auto &repo : args) {
    int try_times = 10;
    for (int j = 0; j < try_times; j++) {
      cinatra::coro_http_client client(pool.get_executor());
      [[maybe_unused]] auto ok = client.init_ssl();
      client.add_header("User-Agent", "cinatra");
      client.add_header("Authorization", auth_token);
      client.add_header("Accept", "application/vnd.github+json");
      client.add_header("X-GitHub-Api-Version", "2022-11-28");
      // https://docs.github.com/en/rest/metrics/traffic?apiVersion=2022-11-28#get-page-views
      std::string url = "https://api.github.com/repos/";
      url.append(repo).append("/traffic/clones");
      auto result = client.get(url);
      if (result.status != 200) {
        std::cerr << result.status << "\n";
        std::cerr << result.resp_body << "\n";
        std::cout << "will retry in 5 seconds\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));
        continue;
      }

      github_clones clones{};
      iguana::from_json(clones, result.resp_body);

      for (auto &item : clones.clones) {
        item.unix_time = get_time_stamp(item.timestamp.substr(0, 10));
      }

      ormpp::dbng<ormpp::sqlite> sqlite;
      size_t pos = repo.find("/") + 1;

      std::string db = repo.substr(pos).append(".db");
      sqlite.connect(db.data());
      sqlite.create_datatable<clone_t>(ormpp_auto_key{"unix_time"});

      sqlite.begin();
      for (auto &item : clones.clones) {
        int r = sqlite.update<clone_t>(item);
        if (r < 0) {
          sqlite.rollback();
          throw std::invalid_argument("update datatable failed");
        }
      }
      sqlite.commit();
      std::cout << cinatra::get_gmt_time_str() << " update table " << repo
                << " successfully!" << std::endl;
      break;
    }
  }
}

// eg: ./github_stat your_token alibaba/yalantinglibs alibaba/async_simple
int main(int argc, char **argv) {
  using namespace cinatra;
  cinatra::http_server server(8);
  bool r = server.listen("0.0.0.0", "9988");
  if (!r) {
    std::cerr << "listen failed\n";
    return -1;
  }

  server.set_http_handler<GET, POST>("/", [](request &, response &res) {
    auto result = read_table();
    res.set_status_and_content(status_type::ok, std::move(result));
  });

  std::thread svr_thd([&server] { server.run(); });

  if (argc < 3) {
    std::cerr << "lack of args: auth_token and repo_owner/repo_name\n";
    std::cout
        << "example: "
           "glb_dgHKs4qDkjm89dsccydQZj0W3RkDcF6w7s8z, alibaba/yalantinglibs\n";
    return -1;
  }

  std::string auth_token = "Bearer ";
  auth_token.append(argv[1]);
  std::vector<std::string> repos;
  for (int i = 2; i < argc; i++) {
    repos.push_back(argv[i]);
  }

  auto &pool = coro_io::g_io_context_pool(argc - 2);

  std::thread thd([&] {
    while (true) {
      update_table(repos, auth_token, pool);
      std::this_thread::sleep_for(std::chrono::hours(3));
    }
  });

  thd.join();
  svr_thd.join();

  return 0;
}
