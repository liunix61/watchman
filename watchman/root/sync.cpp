/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "watchman/PerfSample.h"
#include "watchman/QueryableView.h"
#include "watchman/root/Root.h"
#include "watchman/telemetry/LogEvent.h"
#include "watchman/telemetry/WatchmanStructuredLogger.h"

using namespace watchman;

folly::SemiFuture<folly::Unit> Root::waitForSettle(
    std::chrono::milliseconds settle_period) {
  return view()->waitForSettle(settle_period);
}

CookieSync::SyncResult Root::syncToNow(std::chrono::milliseconds timeout) {
  PerfSample sample("sync_to_now");
  auto root = shared_from_this();
  try {
    auto result = view()->syncToNow(root, timeout);

    auto root_metadata = getRootMetadata();
    auto syncToNow = SyncToNow{
        // MetadataEvent
        {
            // BaseEvent
            {
                root_metadata.root_path.string(), // root
                std::string() // error
            },
            root_metadata.recrawl_count, // recrawl
            root_metadata.case_sensitive, // case_sensitive
            root_metadata.watcher.string() // watcher
        },
        true, // success
        timeout.count() // timeoutms
    };
    getLogger()->logEvent(syncToNow);

    if (sample.finish()) {
      sample.add_root_metadata(root_metadata);
      sample.add_meta(
          "sync_to_now",
          json_object(
              {{"success", json_boolean(true)},
               {"timeoutms", json_integer(timeout.count())}}));
      sample.log();
    }
    return result;
  } catch (const std::exception& exc) {
    auto root_metadata = getRootMetadata();
    auto syncToNow = SyncToNow{
        // MetadataEvent
        {
            // BaseEvent
            {
                root_metadata.root_path.string(), // root
                exc.what() // error
            },
            root_metadata.recrawl_count, // recrawl
            root_metadata.case_sensitive, // case_sensitive
            root_metadata.watcher.string() // watcher
        },
        false, // success
        timeout.count() // timeoutms
    };
    getLogger()->logEvent(syncToNow);

    sample.force_log();
    sample.finish();
    sample.add_root_metadata(root_metadata);
    sample.add_meta(
        "sync_to_now",
        json_object(
            {{"success", json_boolean(false)},
             {"reason", w_string_to_json(exc.what())},
             {"timeoutms", json_integer(timeout.count())}}));
    sample.log();
    throw;
  }
}

/* vim:ts=2:sw=2:et:
 */
