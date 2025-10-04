#!/bin/bash
set -e

SERVER_EXEC="./bin/server"
CONFIG_FILE=$(mktemp)
RESPONSE_HEADERS=$(mktemp)
TEXTVIEW_DIR="my_uploads"
RESPONSE_FILE="response.txt"

mkdir -p "$TEXTVIEW_DIR"

cat > "$CONFIG_FILE" <<EOF
port 80;

location /view TextViewHandler {
  view_dir ./$TEXTVIEW_DIR;
}
EOF

"$SERVER_EXEC" "$CONFIG_FILE" & SERVER_PID=$!

sleep 1

cleanup() {
  kill $SERVER_PID 2>/dev/null || true
  rm -f "$CONFIG_FILE"
  rm -f "$RESPONSE_FILE"
  rm -rf "$TEXTVIEW_DIR"
}
trap cleanup EXIT

echo "=== TextViewHandler Integration Tests ==="

# Test 1: Basic text file
echo "Test 1: Reading a text file..."

# Create a test file
TEST_FILE="./$TEXTVIEW_DIR/test.txt"
touch "$TEST_FILE"
echo "Basic text file test for TextViewHandler." > "$TEST_FILE"

touch "$RESPONSE_FILE"
curl -s -D "$RESPONSE_HEADERS" "http://localhost:80/view/test.txt" -o response.txt

if grep -q "^HTTP/1.1 200" "$RESPONSE_HEADERS"; then
  echo "TextViewHandler: TXT file test passed."
else
  echo "TextViewHandler: TXT file test FAILED."
  cat "$RESPONSE_HEADERS"
  exit 1
fi

if grep -q "Basic text file test for TextViewHandler." "$RESPONSE_FILE"; then
  echo "TextViewHandler: TXT file test passed."
else
  echo "TextViewHandler: TXT file test FAILED."
  cat "$RESPONSE_FILE"
  exit 1
fi

# Test 2: Basic md file
echo "Test 2: Reading a markdown file..."

# Create a test file
TEST_FILE="./$TEXTVIEW_DIR/test.md"
touch "$TEST_FILE"
printf "# Markdown test\nBasic markdown file test for *TextViewHandler*.\n- If it works" > "$TEST_FILE"
cat "$TEST_FILE"

curl -s -D "$RESPONSE_HEADERS" "http://localhost:80/view/test.md" -o response.txt

if grep -q "^HTTP/1.1 200" "$RESPONSE_HEADERS"; then
  echo "TextViewHandler: MD file test passed."
else
  echo "TextViewHandler: MD file test FAILED."
  cat "$RESPONSE_HEADERS"
  exit 1
fi

if grep -q "<h1>Markdown test</h1>" "$RESPONSE_FILE"; then
  echo "TextViewHandler: MD file test passed."
else
  echo "TextViewHandler: MD file test FAILED."
  cat "$RESPONSE_FILE"
  exit 1
fi

if grep -q "<p>Basic markdown file test for <em>TextViewHandler</em>.</p>" "$RESPONSE_FILE"; then
  echo "TextViewHandler: MD file test passed."
else
  echo "TextViewHandler: MD file test FAILED."
  cat "$RESPONSE_FILE"
  exit 1
fi

if grep -q "<li>If it works</li>" "$RESPONSE_FILE"; then
  echo "TextViewHandler: MD file test passed."
else
  echo "TextViewHandler: MD file test FAILED."
  cat "$RESPONSE_FILE"
  exit 1
fi

# Test 3: Basic pdf file
echo "Test 3: Reading a pdf file..."

# Create a test file
cp "./test_pdf/test.pdf" "./$TEXTVIEW_DIR/test.pdf"

curl -s -D "$RESPONSE_HEADERS" "http://localhost:80/view/test.pdf" -o response.txt

if grep -q "^HTTP/1.1 200" "$RESPONSE_HEADERS"; then
  echo "TextViewHandler: PDF file test passed."
else
  echo "TextViewHandler: PDF file test FAILED."
  cat "$RESPONSE_HEADERS"
  exit 1
fi

if grep -q "This is a test pdf." "$RESPONSE_FILE"; then
  echo "TextViewHandler: PDF file test passed."
else
  echo "TextViewHandler: PDF file test FAILED."
  cat "$RESPONSE_FILE"
  exit 1
fi


# Test 4: Unknown file
echo "Test 4: Trying to read an unknown file..."

curl -s -D "$RESPONSE_HEADERS" "http://localhost:80/view/unknown.txt"

if grep -q "^HTTP/1.1 404" "$RESPONSE_HEADERS"; then
  echo "TextViewHandler: Unknown file test passed."
else
  echo "TextViewHandler: Unknown file test FAILED."
  cat "$RESPONSE_HEADERS"
  exit 1
fi

# Test 5: Unsupported file
echo "Test 5: Trying to read an unsupported file..."

# Upload handler prevents upload of non .txt, .pdf, .md files
# What if an unsupported file was in our directory
TEST_FILE="./$TEXTVIEW_DIR/unsupported.log"
touch "$TEST_FILE"
echo -e "Not supported by server" > "$TEST_FILE"

curl -s -D "$RESPONSE_HEADERS" "http://localhost:80/view/unsupported.log"

if grep -q "^HTTP/1.1 501" "$RESPONSE_HEADERS"; then
  echo "TextViewHandler: Unsupported file test passed."
else
  echo "TextViewHandler: Unsupported file test FAILED."
  cat "$RESPONSE_HEADERS"
  exit 1
fi

echo "=== Integration Tests Complete ==="

# Show current state of uploads directory
if [ -d "$TEXTVIEW_DIR" ]; then
    echo ""
    echo "Current uploads directory contents:"
    ls -la "$TEXTVIEW_DIR"
else
    echo ""
    echo "Uploads directory does not exist or is empty"
    exit 1
fi
exit 0

