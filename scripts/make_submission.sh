#!/usr/bin/env bash
#
# 42Tokyo 提出用ディレクトリを生成するスクリプト。
# git管理下のファイルから、CI/エディター設定/テスト一式を除外して dist/ 以下に出力する。
#
# 使い方:
#   ./scripts/make_submission.sh [出力ディレクトリ名]
#
set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel)"
cd "$REPO_ROOT"

NAME="${1:-ircserv-submission}"
DIST_DIR="$REPO_ROOT/dist"
OUT_DIR="$DIST_DIR/$NAME"

EXCLUDES=(
	":(exclude).github"
	":(exclude).githooks"
	":(exclude).vscode"
	":(exclude).clangd"
	":(exclude).clang-format"
	":(exclude).cache"
	":(exclude)tests"
	":(exclude).gitignore"
)

rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"

git archive HEAD -- . "${EXCLUDES[@]}" | tar -x -C "$OUT_DIR"

# 提出物にテストスイートを含めないため、Makefile からも test/unit_test 関連を取り除く。
perl -0pi -e '
	s/^test:[^\n]*\n(?:\t[^\n]*\n)*\n?//m;
	s/^UNIT_TEST_DIR[^\n]*\n(?:UNIT_TEST_\w+[^\n]*\n|UNIT_SRCS[^\n]*\n)*\n?//m;
	s/^unit_test:[^\n]*\n(?:\t[^\n]*\n)*\n?//m;
	s/^\trm -f \$\(UNIT_TEST_BIN\)\n//m;
	s/^(\.PHONY:.*?)\s*\btest\b\s*/$1 /m;
	s/^(\.PHONY:.*?)\s*\bunit_test\b\s*/$1 /m;
' "$OUT_DIR/Makefile"

echo "Generated: $OUT_DIR"
find "$OUT_DIR" -type f | sort
