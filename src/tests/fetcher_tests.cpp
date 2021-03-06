/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <unistd.h>

#include <map>
#include <string>

#include <process/future.hpp>
#include <process/gmock.hpp>
#include <process/gtest.hpp>
#include <process/http.hpp>
#include <process/subprocess.hpp>

#include <stout/gtest.hpp>
#include <stout/net.hpp>
#include <stout/option.hpp>
#include <stout/os.hpp>
#include <stout/strings.hpp>
#include <stout/try.hpp>

#include "slave/containerizer/fetcher.hpp"
#include "slave/flags.hpp"

#include "tests/environment.hpp"
#include "tests/flags.hpp"
#include "tests/utils.hpp"

using namespace mesos;
using namespace mesos::internal;
using namespace mesos::internal::slave;
using namespace mesos::internal::tests;

using namespace process;
using process::Subprocess;
using process::Future;

using std::string;
using std::map;


class FetcherEnvironmentTest : public ::testing::Test {};


TEST_F(FetcherEnvironmentTest, Simple)
{
  CommandInfo commandInfo;
  CommandInfo::URI uri;
  uri.set_value("hdfs:///uri");
  uri.set_executable(false);
  commandInfo.add_uris()->MergeFrom(uri);

  string directory = "/tmp/directory";
  Option<string> user = "user";

  slave::Flags flags;
  flags.frameworks_home = "/tmp/frameworks";
  flags.hadoop_home = "/tmp/hadoop";

  map<string, string> environment =
    fetcher::environment(commandInfo, directory, user, flags);

  EXPECT_EQ(5u, environment.size());
  EXPECT_EQ("hdfs:///uri+0X", environment["MESOS_EXECUTOR_URIS"]);
  EXPECT_EQ(directory, environment["MESOS_WORK_DIRECTORY"]);
  EXPECT_EQ(user.get(), environment["MESOS_USER"]);
  EXPECT_EQ(flags.frameworks_home, environment["MESOS_FRAMEWORKS_HOME"]);
  EXPECT_EQ(flags.hadoop_home, environment["HADOOP_HOME"]);
}


TEST_F(FetcherEnvironmentTest, MultipleURIs)
{
  CommandInfo commandInfo;
  CommandInfo::URI uri;
  uri.set_value("hdfs:///uri1");
  uri.set_executable(false);
  commandInfo.add_uris()->MergeFrom(uri);
  uri.set_value("hdfs:///uri2");
  uri.set_executable(true);
  commandInfo.add_uris()->MergeFrom(uri);

  string directory = "/tmp/directory";
  Option<string> user("user");

  slave::Flags flags;
  flags.frameworks_home = "/tmp/frameworks";
  flags.hadoop_home = "/tmp/hadoop";

  map<string, string> environment =
    fetcher::environment(commandInfo, directory, user, flags);

  EXPECT_EQ(5u, environment.size());
  EXPECT_EQ(
      "hdfs:///uri1+0X hdfs:///uri2+1X", environment["MESOS_EXECUTOR_URIS"]);
  EXPECT_EQ(directory, environment["MESOS_WORK_DIRECTORY"]);
  EXPECT_EQ(user.get(), environment["MESOS_USER"]);
  EXPECT_EQ(flags.frameworks_home, environment["MESOS_FRAMEWORKS_HOME"]);
  EXPECT_EQ(flags.hadoop_home, environment["HADOOP_HOME"]);
}


TEST_F(FetcherEnvironmentTest, NoUser)
{
  CommandInfo commandInfo;
  CommandInfo::URI uri;
  uri.set_value("hdfs:///uri");
  uri.set_executable(false);
  commandInfo.add_uris()->MergeFrom(uri);

  string directory = "/tmp/directory";

  slave::Flags flags;
  flags.frameworks_home = "/tmp/frameworks";
  flags.hadoop_home = "/tmp/hadoop";

  map<string, string> environment =
    fetcher::environment(commandInfo, directory, None(), flags);

  EXPECT_EQ(4u, environment.size());
  EXPECT_EQ("hdfs:///uri+0X", environment["MESOS_EXECUTOR_URIS"]);
  EXPECT_EQ(directory, environment["MESOS_WORK_DIRECTORY"]);
  EXPECT_EQ(flags.frameworks_home, environment["MESOS_FRAMEWORKS_HOME"]);
  EXPECT_EQ(flags.hadoop_home, environment["HADOOP_HOME"]);
}


TEST_F(FetcherEnvironmentTest, EmptyHadoop)
{
  CommandInfo commandInfo;
  CommandInfo::URI uri;
  uri.set_value("hdfs:///uri");
  uri.set_executable(false);
  commandInfo.add_uris()->MergeFrom(uri);

  string directory = "/tmp/directory";
  Option<string> user = "user";

  slave::Flags flags;
  flags.frameworks_home = "/tmp/frameworks";
  flags.hadoop_home = "";

  map<string, string> environment =
    fetcher::environment(commandInfo, directory, user, flags);

  EXPECT_EQ(4u, environment.size());
  EXPECT_EQ("hdfs:///uri+0X", environment["MESOS_EXECUTOR_URIS"]);
  EXPECT_EQ(directory, environment["MESOS_WORK_DIRECTORY"]);
  EXPECT_EQ(user.get(), environment["MESOS_USER"]);
  EXPECT_EQ(flags.frameworks_home, environment["MESOS_FRAMEWORKS_HOME"]);
}


TEST_F(FetcherEnvironmentTest, NoHadoop)
{
  CommandInfo commandInfo;
  CommandInfo::URI uri;
  uri.set_value("hdfs:///uri");
  uri.set_executable(false);
  commandInfo.add_uris()->MergeFrom(uri);

  string directory = "/tmp/directory";
  Option<string> user = "user";

  slave::Flags flags;
  flags.frameworks_home = "/tmp/frameworks";

  map<string, string> environment =
    fetcher::environment(commandInfo, directory, user, flags);

  EXPECT_EQ(4u, environment.size());
  EXPECT_EQ("hdfs:///uri+0X", environment["MESOS_EXECUTOR_URIS"]);
  EXPECT_EQ(directory, environment["MESOS_WORK_DIRECTORY"]);
  EXPECT_EQ(user.get(), environment["MESOS_USER"]);
  EXPECT_EQ(flags.frameworks_home, environment["MESOS_FRAMEWORKS_HOME"]);
}


TEST_F(FetcherEnvironmentTest, NoExtractNoExecutable)
{
  CommandInfo commandInfo;
  CommandInfo::URI uri;
  uri.set_value("hdfs:///uri");
  uri.set_executable(false);
  uri.set_extract(false);
  commandInfo.add_uris()->MergeFrom(uri);

  string directory = "/tmp/directory";
  Option<string> user = "user";

  slave::Flags flags;
  flags.frameworks_home = "/tmp/frameworks";
  flags.hadoop_home = "/tmp/hadoop";

  map<string, string> environment =
    fetcher::environment(commandInfo, directory, user, flags);
  EXPECT_EQ(5u, environment.size());
  EXPECT_EQ("hdfs:///uri+0N", environment["MESOS_EXECUTOR_URIS"]);
  EXPECT_EQ(directory, environment["MESOS_WORK_DIRECTORY"]);
  EXPECT_EQ(user.get(), environment["MESOS_USER"]);
  EXPECT_EQ(flags.frameworks_home, environment["MESOS_FRAMEWORKS_HOME"]);
  EXPECT_EQ(flags.hadoop_home, environment["HADOOP_HOME"]);
}


TEST_F(FetcherEnvironmentTest, NoExtractExecutable)
{
  CommandInfo commandInfo;
  CommandInfo::URI uri;
  uri.set_value("hdfs:///uri");
  uri.set_executable(true);
  uri.set_extract(false);
  commandInfo.add_uris()->MergeFrom(uri);

  string directory = "/tmp/directory";
  Option<string> user = "user";

  slave::Flags flags;
  flags.frameworks_home = "/tmp/frameworks";
  flags.hadoop_home = "/tmp/hadoop";

  map<string, string> environment =
    fetcher::environment(commandInfo, directory, user, flags);
  EXPECT_EQ(5u, environment.size());
  EXPECT_EQ("hdfs:///uri+1N", environment["MESOS_EXECUTOR_URIS"]);
  EXPECT_EQ(directory, environment["MESOS_WORK_DIRECTORY"]);
  EXPECT_EQ(user.get(), environment["MESOS_USER"]);
  EXPECT_EQ(flags.frameworks_home, environment["MESOS_FRAMEWORKS_HOME"]);
  EXPECT_EQ(flags.hadoop_home, environment["HADOOP_HOME"]);
}


class FetcherTest : public TemporaryDirectoryTest {};


TEST_F(FetcherTest, FileURI)
{
  string fromDir = path::join(os::getcwd(), "from");
  ASSERT_SOME(os::mkdir(fromDir));
  string testFile = path::join(fromDir, "test");
  EXPECT_FALSE(os::write(testFile, "data").isError());

  string localFile = path::join(os::getcwd(), "test");
  EXPECT_FALSE(os::exists(localFile));

  map<string, string> env;

  env["MESOS_EXECUTOR_URIS"] = "file://" + testFile + "+0N";
  env["MESOS_WORK_DIRECTORY"] = os::getcwd();

  Try<Subprocess> fetcherProcess =
    process::subprocess(
      path::join(mesos::internal::tests::flags.build_dir, "src/mesos-fetcher"),
      env);

  ASSERT_SOME(fetcherProcess);
  Future<Option<int>> status = fetcherProcess.get().status();

  AWAIT_READY(status);
  ASSERT_SOME(status.get());

  EXPECT_EQ(0, status.get().get());
  EXPECT_TRUE(os::exists(localFile));
}


TEST_F(FetcherTest, FilePath)
{
  string fromDir = path::join(os::getcwd(), "from");
  ASSERT_SOME(os::mkdir(fromDir));
  string testFile = path::join(fromDir, "test");
  EXPECT_FALSE(os::write(testFile, "data").isError());

  string localFile = path::join(os::getcwd(), "test");
  EXPECT_FALSE(os::exists(localFile));

  map<string, string> env;

  env["MESOS_EXECUTOR_URIS"] = testFile + "+0N";
  env["MESOS_WORK_DIRECTORY"] = os::getcwd();

  Try<Subprocess> fetcherProcess =
    process::subprocess(
      path::join(mesos::internal::tests::flags.build_dir, "src/mesos-fetcher"),
      env);

  ASSERT_SOME(fetcherProcess);
  Future<Option<int>> status = fetcherProcess.get().status();

  AWAIT_READY(status);
  ASSERT_SOME(status.get());

  EXPECT_EQ(0, status.get().get());
  EXPECT_TRUE(os::exists(localFile));
}


class HttpProcess : public Process<HttpProcess>
{
public:
  HttpProcess()
  {
    route("/help", None(), &HttpProcess::index);
  }

  Future<http::Response> index(const http::Request& request)
  {
    return http::OK();
  }
};


TEST_F(FetcherTest, OSNetUriTest)
{
  HttpProcess process;

  spawn(process);

  string url = "http://" + net::getHostname(process.self().ip).get() +
                ":" + stringify(process.self().port) + "/help";

  string localFile = path::join(os::getcwd(), "help");
  EXPECT_FALSE(os::exists(localFile));

  map<string, string> env;

  env["MESOS_EXECUTOR_URIS"] = url + "+0N";
  env["MESOS_WORK_DIRECTORY"] = os::getcwd();

  Try<Subprocess> fetcherProcess =
    process::subprocess(
      path::join(mesos::internal::tests::flags.build_dir, "src/mesos-fetcher"),
      env);

  ASSERT_SOME(fetcherProcess);
  Future<Option<int>> status = fetcherProcess.get().status();

  AWAIT_READY(status);
  ASSERT_SOME(status.get());

  EXPECT_EQ(0, status.get().get());
  EXPECT_TRUE(os::exists(localFile));
}


TEST_F(FetcherTest, FileLocalhostURI)
{
  string fromDir = path::join(os::getcwd(), "from");
  ASSERT_SOME(os::mkdir(fromDir));
  string testFile = path::join(fromDir, "test");
  EXPECT_FALSE(os::write(testFile, "data").isError());

  string localFile = path::join(os::getcwd(), "test");
  EXPECT_FALSE(os::exists(localFile));

  map<string, string> env;

  env["MESOS_EXECUTOR_URIS"] = path::join("file://localhost", testFile) + "+0N";
  env["MESOS_WORK_DIRECTORY"] = os::getcwd();

  Try<Subprocess> fetcherProcess =
    process::subprocess(
      path::join(mesos::internal::tests::flags.build_dir, "src/mesos-fetcher"),
      env);

  ASSERT_SOME(fetcherProcess);
  Future<Option<int>> status = fetcherProcess.get().status();

  AWAIT_READY(status);
  ASSERT_SOME(status.get());

  EXPECT_EQ(0, status.get().get());
  EXPECT_TRUE(os::exists(localFile));
}


TEST_F(FetcherTest, NoExtractNotExecutable)
{
  // First construct a temporary file that can be fetched.
  Try<string> path = os::mktemp();

  ASSERT_SOME(path);

  CommandInfo commandInfo;
  CommandInfo::URI* uri = commandInfo.add_uris();
  uri->set_value(path.get());
  uri->set_executable(false);
  uri->set_extract(false);

  Option<int> stdout = None();
  Option<int> stderr = None();

  // Redirect mesos-fetcher output if running the tests verbosely.
  if (tests::flags.verbose) {
    stdout = STDOUT_FILENO;
    stderr = STDERR_FILENO;
  }

  slave::Flags flags;
  flags.launcher_dir = path::join(tests::flags.build_dir, "src");

  Future<Option<int>> run =
    fetcher::run(commandInfo, os::getcwd(), None(), flags, stdout, stderr);

  AWAIT_READY(run);
  EXPECT_SOME_EQ(0, run.get());

  Try<string> basename = os::basename(path.get());

  ASSERT_SOME(basename);

  Try<os::Permissions> permissions = os::permissions(basename.get());

  ASSERT_SOME(permissions);
  EXPECT_FALSE(permissions.get().owner.x);
  EXPECT_FALSE(permissions.get().group.x);
  EXPECT_FALSE(permissions.get().others.x);

  ASSERT_SOME(os::rm(path.get()));
}


TEST_F(FetcherTest, NoExtractExecutable)
{
  // First construct a temporary file that can be fetched.
  Try<string> path = os::mktemp();

  ASSERT_SOME(path);

  CommandInfo commandInfo;
  CommandInfo::URI* uri = commandInfo.add_uris();
  uri->set_value(path.get());
  uri->set_executable(true);
  uri->set_extract(false);

  Option<int> stdout = None();
  Option<int> stderr = None();

  // Redirect mesos-fetcher output if running the tests verbosely.
  if (tests::flags.verbose) {
    stdout = STDOUT_FILENO;
    stderr = STDERR_FILENO;
  }

  slave::Flags flags;
  flags.launcher_dir = path::join(tests::flags.build_dir, "src");

  Future<Option<int>> run =
    fetcher::run(commandInfo, os::getcwd(), None(), flags, stdout, stderr);

  AWAIT_READY(run);
  EXPECT_SOME_EQ(0, run.get());

  Try<string> basename = os::basename(path.get());

  ASSERT_SOME(basename);

  Try<os::Permissions> permissions = os::permissions(basename.get());

  ASSERT_SOME(permissions);
  EXPECT_TRUE(permissions.get().owner.x);
  EXPECT_TRUE(permissions.get().group.x);
  EXPECT_TRUE(permissions.get().others.x);

  ASSERT_SOME(os::rm(path.get()));
}


TEST_F(FetcherTest, ExtractNotExecutable)
{
  // First construct a temporary file that can be fetched and archive
  // with tar  gzip.
  Try<string> path = os::mktemp();

  ASSERT_SOME(path);

  ASSERT_SOME(os::write(path.get(), "hello world"));

  // TODO(benh): Update os::tar so that we can capture or ignore
  // stdout/stderr output.

  ASSERT_SOME(os::tar(path.get(), path.get() + ".tar.gz"));

  CommandInfo commandInfo;
  CommandInfo::URI* uri = commandInfo.add_uris();
  uri->set_value(path.get() + ".tar.gz");
  uri->set_executable(false);
  uri->set_extract(true);

  Option<int> stdout = None();
  Option<int> stderr = None();

  // Redirect mesos-fetcher output if running the tests verbosely.
  if (tests::flags.verbose) {
    stdout = STDOUT_FILENO;
    stderr = STDERR_FILENO;
  }

  slave::Flags flags;
  flags.launcher_dir = path::join(tests::flags.build_dir, "src");

  Future<Option<int>> run =
    fetcher::run(commandInfo, os::getcwd(), None(), flags, stdout, stderr);

  AWAIT_READY(run);
  EXPECT_SOME_EQ(0, run.get());

  ASSERT_TRUE(os::exists(path::join(".", path.get())));

  ASSERT_SOME_EQ("hello world", os::read(path::join(".", path.get())));

  Try<os::Permissions> permissions =
    os::permissions(path::join(".", path.get()));

  ASSERT_SOME(permissions);
  EXPECT_FALSE(permissions.get().owner.x);
  EXPECT_FALSE(permissions.get().group.x);
  EXPECT_FALSE(permissions.get().others.x);

  ASSERT_SOME(os::rm(path.get()));
}
