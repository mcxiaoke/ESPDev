import subprocess

revision = subprocess.check_output(
    ["git", "rev-parse", "--short", "HEAD"]).strip()
# git rev-parse --short HEAD
# revision = subprocess.check_output(["git", "describe", "--tags", "--always"]).strip()
print("-DPIO_SRC_REV=%s" % revision.decode('utf8'))
