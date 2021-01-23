import subprocess
from datetime import datetime

build_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
revision = subprocess.check_output(
    ["git", "rev-parse", "--short", "HEAD"]).strip()
# git rev-parse --short HEAD
# revision = subprocess.check_output(["git", "describe", "--tags", "--always"]).strip()
# print("\n    -DPIO_SRC_REV=\"{}\"\n    -DPIO_BUILD_TIME=\"{}\"".format(revision.decode('utf8'), build_time))
print("-DPIO_SRC_REV={}".format(revision.decode('utf8')))
