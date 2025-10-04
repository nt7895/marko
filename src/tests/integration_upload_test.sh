#!/bin/bash

# Integration test script for upload functionality
# This script should be run after the server is started
# Run from project root directory: ./tests/integration_upload_test.sh

SERVER_URL="http://localhost:80"  # Changed from 8080 to 80 to match config
UPLOAD_ENDPOINT="${SERVER_URL}/upload"
UPLOADS_DIR="./uploads"

echo "=== Upload Handler Integration Tests ==="

# Test 1: Upload a small valid file
echo "Test 1: Uploading a small valid file..."

# Create a test file
TEST_FILE="test_upload.txt"
echo "This is a test file for upload integration testing." > "${TEST_FILE}"

# Upload using curl
RESPONSE=$(curl -s -w "%{http_code}" -o response.html \
  -F "file=@${TEST_FILE}" \
  -F "course_code=CS130" \
  -F "title=Integration Test File" \
  "${UPLOAD_ENDPOINT}")

HTTP_CODE="${RESPONSE: -3}"

if [ "$HTTP_CODE" -eq 200 ]; then
    echo "✓ Small file upload successful (HTTP $HTTP_CODE)"
    
    # Check if file exists in uploads directory
    if [ -d "$UPLOADS_DIR" ] && [ "$(ls -A $UPLOADS_DIR)" ]; then
        echo "✓ File found in uploads directory"
        ls -la "$UPLOADS_DIR"
    else
        echo "✗ No files found in uploads directory"
    fi
else
    echo "✗ Small file upload failed (HTTP $HTTP_CODE)"
    cat response.html
fi

echo ""

# Test 2: Upload a file larger than 10MB
echo "Test 2: Uploading a file larger than 10MB..."

# Create a large test file (11MB)
LARGE_FILE="large_test.txt"
dd if=/dev/zero of="${LARGE_FILE}" bs=1M count=11 2>/dev/null

# Upload using curl
LARGE_RESPONSE=$(curl -s -w "%{http_code}" -o large_response.html \
  -F "file=@${LARGE_FILE}" \
  -F "course_code=CS130" \
  -F "title=Large Test File" \
  "${UPLOAD_ENDPOINT}")

LARGE_HTTP_CODE="${LARGE_RESPONSE: -3}"

if [ "$LARGE_HTTP_CODE" -eq 400 ]; then
    echo "✓ Large file upload correctly rejected (HTTP $LARGE_HTTP_CODE)"
    
    # Check if error message is present
    if grep -q "File validation failed" large_response.html; then
        echo "✓ Correct error message displayed"
    else
        echo "? Unexpected error message:"
        cat large_response.html
    fi
else
    echo "✗ Large file upload should have been rejected but got HTTP $LARGE_HTTP_CODE"
    cat large_response.html
fi

echo ""

# Test 3: Test PDF upload (if supported)
echo "Test 3: Testing PDF upload support..."

# Create a dummy PDF file (just for extension testing)
PDF_FILE="test.pdf"
echo "%PDF-1.4
1 0 obj
<<
/Type /Catalog
/Pages 2 0 R
>>
endobj
2 0 obj
<<
/Type /Pages
/Kids [3 0 R]
/Count 1
>>
endobj
3 0 obj
<<
/Type /Page
/Parent 2 0 R
/MediaBox [0 0 612 792]
>>
endobj
xref
0 4
0000000000 65535 f 
0000000010 00000 n 
0000000053 00000 n 
0000000099 00000 n 
trailer
<<
/Size 4
/Root 1 0 R
>>
startxref
161
%%EOF" > "${PDF_FILE}"

PDF_RESPONSE=$(curl -s -w "%{http_code}" -o pdf_response.html \
  -F "file=@${PDF_FILE}" \
  -F "course_code=CS130" \
  -F "title=PDF Test File" \
  "${UPLOAD_ENDPOINT}")

PDF_HTTP_CODE="${PDF_RESPONSE: -3}"

if [ "$PDF_HTTP_CODE" -eq 200 ]; then
    echo "✓ PDF upload successful (HTTP $PDF_HTTP_CODE)"
else
    echo "✗ PDF upload failed (HTTP $PDF_HTTP_CODE)"
    if grep -q "File validation failed" pdf_response.html; then
        echo "  Note: PDF support may not be implemented yet"
    fi
fi

echo ""

# Test 4: Test GET request for upload form
echo "Test 4: Testing GET request for upload form..."

FORM_RESPONSE=$(curl -s -w "%{http_code}" -o form_response.html "${UPLOAD_ENDPOINT}")
FORM_HTTP_CODE="${FORM_RESPONSE: -3}"

if [ "$FORM_HTTP_CODE" -eq 200 ]; then
    echo "✓ Upload form retrieved successfully (HTTP $FORM_HTTP_CODE)"
    
    if grep -q "UCLA Notes Upload" form_response.html; then
        echo "✓ Form contains expected content"
    else
        echo "? Form content may be unexpected"
    fi
else
    echo "✗ Failed to retrieve upload form (HTTP $FORM_HTTP_CODE)"
fi

echo ""

# Test 5: Test invalid file extension
echo "Test 5: Testing invalid file extension (.exe)..."

# Create a test file with invalid extension
INVALID_FILE="test.exe"
echo "This should be rejected" > "${INVALID_FILE}"

INVALID_RESPONSE=$(curl -s -w "%{http_code}" -o invalid_response.html \
  -F "file=@${INVALID_FILE}" \
  -F "course_code=CS130" \
  -F "title=Invalid File Test" \
  "${UPLOAD_ENDPOINT}")

INVALID_HTTP_CODE="${INVALID_RESPONSE: -3}"

if [ "$INVALID_HTTP_CODE" -eq 400 ]; then
    echo "✓ Invalid file extension correctly rejected (HTTP $INVALID_HTTP_CODE)"
else
    echo "✗ Invalid file extension should have been rejected but got HTTP $INVALID_HTTP_CODE"
    cat invalid_response.html
fi

echo ""

# Clean up test files
echo "Cleaning up test files..."
rm -f "${TEST_FILE}" "${LARGE_FILE}" "${PDF_FILE}" "${INVALID_FILE}"
rm -f response.html large_response.html pdf_response.html form_response.html invalid_response.html

echo "=== Integration Tests Complete ==="

# Show current state of uploads directory
if [ -d "$UPLOADS_DIR" ]; then
    echo ""
    echo "Current uploads directory contents:"
    ls -la "$UPLOADS_DIR"
else
    echo ""
    echo "Uploads directory does not exist or is empty"
fi
