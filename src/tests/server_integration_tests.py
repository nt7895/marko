#!/usr/bin/env python3

import sys
import time
import subprocess
import requests
import os
import atexit
import urllib.parse
import json
from multiprocessing import Process, Queue
import time
import socket

class ServerIntegrationTest:
    def __init__(self):
        self.server_binary = "./bin/server"
        self.test_port = 80
        self.server_process = None
        self.all_success = True
        self.config_file = 'my_config'
        # parse data_path from config:
        self.data_path = None
        with open(self.config_file, 'r') as f:
            for line in f:
                line = line.strip()
                if line.startswith("data_path"):
                    # e.g. "data_path ./database;"
                    parts = line.split()
                    if len(parts) >= 2:
                        self.data_path = parts[1].rstrip(';')
                    break
        if not self.data_path:
            raise RuntimeError("Could not find data_path in " + self.config_file)
        
        # Register cleanup function
        atexit.register(self.cleanup)
    
    def cleanup(self):
        if self.server_process:
            print(f"Shutting down server (PID: {self.server_process.pid})...")
            try:
                self.server_process.terminate()
                self.server_process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.server_process.kill()

            # Test 2: Check server logs are captured
            output = self.server_process.communicate()[0]
            output_lines = output.decode('utf-8').split("\n")

            # Need to change this if you modify log messages
            expected_messages = ['Server config file successfully parsed', 
                               'Server started at port 80', 
                               'CONNECTED', 
                               'GET / HTTP/1.1', 
                               'HTTP/1.1 200 OK', 
                               'Server closed']
            
            # print(output_lines)
            # print(expected_messages)
            
            # Can't test here because output log is content of all http requests need to split to run_test
            
            # for i in range(len(output_lines)-1): # there's an extra newline at end 
            #     if output_lines[i][len(output_lines[i]) - len(expected_messages[i]):] != expected_messages[i]:
            #         self.all_success = False
            
            print("Server Logs:")
            print(output.decode('utf-8'))
            self.server_process = None
    
    def start_server(self):
        # Start the server as a subprocess
        print(f"Starting web server on port {self.test_port}...")
        self.server_process = subprocess.Popen([self.server_binary, self.config_file], stdout=subprocess.PIPE)

        # Wait for server to start
        print(f"Waiting for server to start (PID: {self.server_process.pid})...")
        time.sleep(1)
        
        # Check if server is still running
        if self.server_process.poll() is not None:
            print("ERROR: Server failed to start!")
            sys.exit(1)
        
        print("Server started successfully.")
    
    def run_test(self, test_name, url, expected_content=None, expected_status_code=200):
        print(f"Running test: {test_name}")
        
        # Make the request and capture the response
        try:
            response = requests.get(url, timeout=3)
            # Check status code if specified
            if response.status_code != expected_status_code:
                print(f"ERROR: Test {test_name} failed: Expected status code {expected_status_code}, got {response.status_code}")
                return False
                
            # If no expected content was provided, we're just checking the status code
            if expected_content is None:
                print(f"PASSED: {test_name} - Status code {response.status_code} as expected")
                return True
                
            # Compare with expected output, line by line
            expected_content = expected_content.splitlines()
            response_content = response.text.splitlines()
            if len(expected_content) != len(response_content):
                print(f"ERROR: Test {test_name} failed: Expected {len(expected_content)} lines, got {len(response_content)} lines.")
                return False
            for i, (expected_line, response_line) in enumerate(zip(expected_content, response_content)):
                if expected_line != response_line:
                    print(f"ERROR: Test {test_name} failed at line {i + 1}: Expected '{expected_line}', got '{response_line}'")
                    return False
            
            print(f"PASSED: {test_name}")
            return True
            
        except requests.RequestException as e:
            print(f"ERROR: Request failed: {e}")
            return False
    
    # ------------------------------------------------------------------
    # helpers for POST / PUT / DELETE that mirror run_test()’s behaviour
    # ------------------------------------------------------------------
    def post_test(self, test_name, url, payload, *, expected_status=200):
        print(f"Running test: {test_name}")
        try:
            r = requests.post(url, json=payload, timeout=3)
            if r.status_code != expected_status:
                print(f"ERROR: {test_name} – expected {expected_status}, got {r.status_code}")
                return False
            print(f"PASSED: {test_name}")
            return r      # caller may inspect .json()
        except requests.RequestException as e:
            print(f"ERROR: {test_name} – request failed: {e}")
            return False

    def put_test(self, test_name, url, payload, *, expected_status=200):
        print(f"Running test: {test_name}")
        try:
            r = requests.put(url, json=payload, timeout=3)
            if r.status_code != expected_status:
                print(f"ERROR: {test_name} – expected {expected_status}, got {r.status_code}")
                return False
            print(f"PASSED: {test_name}")
            return r
        except requests.RequestException as e:
            print(f"ERROR: {test_name} – request failed: {e}")
            return False

    def delete_test(self, test_name, url, *, expected_status=200):
        print(f"Running test: {test_name}")
        try:
            r = requests.delete(url, timeout=3)
            if r.status_code != expected_status:
                print(f"ERROR: {test_name} – expected {expected_status}, got {r.status_code}")
                return False
            print(f"PASSED: {test_name}")
            return True
        except requests.RequestException as e:
            print(f"ERROR: {test_name} – request failed: {e}")
            return False
        
    def multithread_sleep_test(self, test_name, url, queue, expected_status_code=200):
        print(f"Running test: {test_name}")

        try:
            # sleep will block for 5 seconds so wait +2 seconds before timeout
            start_time = time.time()
            response = requests.get(url, timeout=7)
            end_time = time.time()
            queue.put({"start": start_time, "end": end_time})

            # Check status code if specified
            if response.status_code != expected_status_code:
                print(f"ERROR: Test {test_name} failed: Expected status code {expected_status_code}, got {response.status_code}")
                return False
            
            print(f"PASSED: {test_name}")
            return True
            
        except requests.RequestException as e:
            print(f"ERROR: Request failed: {e}")
            return False
        
    def multithread_echo_test(self, test_name, url, queue, expected_status_code=200):
        print(f"Running test: {test_name}")

        try:
            start_time = time.time()
            response = requests.get(url, timeout=3)
            end_time = time.time()
            queue.put({"start": start_time, "end": end_time})

            # Check status code if specified
            if response.status_code != expected_status_code:
                print(f"ERROR: Test {test_name} failed: Expected status code {expected_status_code}, got {response.status_code}")
                return False
            
            # echo should respond immediately
            if end_time - start_time < 3:
                print(f"PASSED: {test_name}")
            else:
                print(f"FAILED: {test_name}")
                
            return True
            
        except requests.RequestException as e:
            print(f"ERROR: Request failed: {e}")
            return False

    def malformed_req_test(self, test_name, url, expected_content=None):
        print(f"Running test: {test_name}")
        
        # Make the request and capture the response
        try:

            # Malformed HTTP request: invalid method and missing Host header
            malformed_request = b"GET "

            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(("localhost", self.test_port))
            sock.send(malformed_request)
            response = sock.recv(4096).decode()
            sock.close()

            expected_content = expected_content.splitlines()
            response_content = response.splitlines()
            for i, (expected_line, response_line) in enumerate(zip(expected_content, response_content)):
                if expected_line != response_line:
                    print(f"ERROR: Test {test_name} failed at line {i + 1}: Expected '{expected_line}', got '{response_line}'")
                    return False
            
            print(f"PASSED: {test_name}")
            return True
            
        except requests.RequestException as e:
            print(f"ERROR: Request failed: {e}")
            return False
        
    def run_tests(self):
        print("=== Web Server Integration Tests ===")
        
        # Start the server
        self.start_server()

        # Test 1: Basic GET request to the root path returns 404
        test_url = f"http://localhost:{self.test_port}/"
        if not self.run_test("basic_get", test_url,
                            expected_content=None,
                            expected_status_code=404):
            self.all_success = False

        # Test 2: Echo handler
        test_url = f"http://localhost:{self.test_port}/echo/test"
        ua = f"python-requests/{requests.__version__}"
        expected_echo_content = (
            "GET /echo/test HTTP/1.1\r\n"
            "Host: localhost\r\n"
            f"User-Agent: {ua}\r\n"
            "Accept-Encoding: gzip, deflate\r\n"
            "Accept: */*\r\n"
            "Connection: keep-alive\n\n"
        )
        if not self.run_test("echo_handler", test_url, expected_echo_content):
            self.all_success = False
            
        # Test 3: Static handler (assuming /static points to a valid directory)
        test_url = f"http://localhost:{self.test_port}/usr/html_test.html"
        if not self.run_test("static_handler", test_url, expected_content=None):
            self.all_success = False
            
        # Test 4: 404 Not Found
        test_url = f"http://localhost:{self.test_port}/nonexistent"
        if not self.run_test("not_found", test_url, expected_status_code=404, expected_content=None):
            self.all_success = False
        
        # ------------------------------------------------------------------
        # New CRUD API integration tests (/api prefix handled by APIHandler)
        # ------------------------------------------------------------------

        base = f"http://localhost:{self.test_port}"

        # 5: POST /api/Shoes  → create entity, expect {"id": 1}
        r = self.post_test("api_create",
                           f"{base}/api/Shoes",
                           {"brand": "Nike", "size": 42})
        if not r: self.all_success = False
        else:
            new_id = r.json().get("id")
            if new_id != 1:
                print(f"ERROR: api_create – expected id 1, got {new_id}")
                self.all_success = False

        # 6: GET /api/Shoes/1  → retrieve
        if not self.run_test("api_retrieve",
                             f"{base}/api/Shoes/1",
                             expected_content=json.dumps({"brand": "Nike", "size": 42}),
                             expected_status_code=200):
            self.all_success = False

        # 7: PUT /api/Shoes/1  → update
        if not self.put_test("api_update",
                             f"{base}/api/Shoes/1",
                             {"brand": "Nike", "size": 43}):
            self.all_success = False

        # 8: GET /api/Shoes/1 again → size should be 43
        if not self.run_test("api_retrieve_after_update",
                             f"{base}/api/Shoes/1",
                             expected_content=json.dumps({"brand": "Nike", "size": 43}),
                             expected_status_code=200):
            self.all_success = False

        # 9: GET /api/Shoes (list) → [1]
        if not self.run_test("api_list",
                             f"{base}/api/Shoes",
                             expected_content="[1]",
                             expected_status_code=200):
            self.all_success = False

        # 10: Persistence test: ensure the file is on disk
        db_dir = os.path.join(self.data_path, "Shoes")
        file_path = os.path.join(db_dir, "1")
        if not os.path.exists(file_path):
            print(f"ERROR: Persistence test: expected '{file_path}' to exist")
            self.all_success = False
        else:
            print("PASSED: Persistence test - write to disk verified")

        # 11: Persistence *across restart* of the server process
        print(">>> Restarting server to verify persistence across restarts...")
        # shut down the current server process
        try:
            self.server_process.terminate()
            self.server_process.wait(timeout=2)
        except Exception:
            self.server_process.kill()
        # give the OS a moment to free up port 80
        time.sleep(1)

        # start it again
        self.start_server()

        # re-GET the same entity
        restart_url = f"{base}/api/Shoes/1"
        expected_after_restart = json.dumps({"brand": "Nike", "size": 43})
        if not self.run_test("api_retrieve_after_restart",
                             restart_url,
                             expected_content=expected_after_restart,
                             expected_status_code=200):
            self.all_success = False
        else:
            print("PASSED: Persistence across restart verified")

        # 12: DELETE /api/Shoes/1
        if not self.delete_test("api_delete",
                                f"{base}/api/Shoes/1"):
            self.all_success = False

        # 13: GET /api/Shoes/1 should now be 404
        if not self.run_test("api_retrieve_deleted",
                             f"{base}/api/Shoes/1",
                             expected_content=None,
                             expected_status_code=404):
            self.all_success = False
        
        # 14: Multithreading 
        sleep_url = f"http://localhost:{self.test_port}/sleep"
        echo_url = f"http://localhost:{self.test_port}/echo"
        expected_echo_content = "GET /echo/test HTTP/1.1\r\nHost: localhost\r\nUser-Agent: python-requests/2.31.0\r\nAccept-Encoding: gzip, deflate\r\nAccept: */*\r\nConnection: keep-alive\n\n"
        
        # stores start/end benchmarks for requests
        sleep_queue = Queue()
        echo_queue = Queue()

        sleep_req = Process(target=self.multithread_sleep_test, args=("multithreading_sleep_handler", sleep_url, sleep_queue))
        echo_req = Process(target=self.multithread_echo_test, args=("multithreading_echo_handler", echo_url, echo_queue))

        sleep_req.start()
        time.sleep(0.05) # ensures echo starts after sleep
        echo_req.start()
        
        # wait until both processes finish
        processes = [sleep_req, echo_req]
        for p in processes:
            p.join()

        echo_results = echo_queue.get()
        sleep_results = sleep_queue.get()
        
        # Checks that echo came back faster than sleep
        if echo_results["start"] > sleep_results["start"] and echo_results["end"] < sleep_results["end"]:
            print("PASSED: Requests were successfully multithreaded")
        else:
            print("FAILED: Requests failed to be multithreaded")
            self.all_success = False
            
        # 15: Health Handler
        test_url = f"http://localhost:{self.test_port}/health"
        expected_health_content = "HTTP/1.1 200 OK\r\nHost: localhost\r\nUser-Agent: python-requests/2.31.0\r\nAccept-Encoding: gzip, deflate\r\nAccept: */*\r\nConnection: keep-alive\n\n"
        if not self.run_test("health_handler", test_url, expected_content=None):
            self.all_success = False
        
        # 16: Malformed Requests
        test_url = f"http://localhost:{self.test_port}/echo"
        e = "HTTP/1.1 400 Bad Request\r\nContent-Length: 46\r\nContent-Type: text/plain\r\n\r\nRequest is malformed and cannot be processed\r\n"
        if not self.malformed_req_test("malformed_requests", test_url, e):
            self.all_success = False


        # Shut down the server
        self.cleanup()
        
        # Print final results
        if self.all_success:
            print("=== All tests PASSED ===")
        else:
            print("=== Some tests FAILED ===")
        
        return 0 if self.all_success else 1

def url_to_raw_request_method1(url):
    """
    Convert a URL to a raw HTTP request string using low-level components.
    """
    # Parse the URL
    parsed_url = urllib.parse.urlparse(url)
    
    # Extract components
    host = parsed_url.netloc.split(':')[0]
    path = parsed_url.path or "/"
    if parsed_url.query:
        path += f"?{parsed_url.query}"
    
    # Build the request manually
    request_lines = [
        f"GET {path} HTTP/1.1",
        f"Host: {host}",
        "User-Agent: python-requests/2.31.0",
        "Accept-Encoding: gzip, deflate",
        "Accept: */*",
        "Connection: keep-alive",
        "",  # Empty line to indicate end of headers
        ""   # Empty line at end
    ]
    return "\n".join(request_lines)

if __name__ == "__main__":
    test = ServerIntegrationTest()
    sys.exit(test.run_tests())