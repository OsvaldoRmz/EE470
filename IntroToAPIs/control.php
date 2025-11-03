<?php
$filename = "results.txt";

if ($_SERVER["REQUEST_METHOD"] === "PUT") {
  // Read the raw PUT data from the HTTP request
  $data = trim(file_get_contents("php://input"));

  // Accept only "ON" or "OFF"
  if ($data === "ON" || $data === "OFF") {
    // Create a JSON object like: {"led":"ON"}
    $json = json_encode(["led" => $data], JSON_PRETTY_PRINT);

    // Write it to results.txt
    file_put_contents($filename, $json);

    echo "OK";
  } else {
    http_response_code(400);
    echo "Invalid input. Use ON or OFF.";
  }
  exit;
}

if ($_SERVER["REQUEST_METHOD"] === "GET") {
  // Return the current LED state in JSON format
  header("Content-Type: application/json; charset=utf-8");

  if (file_exists($filename)) {
    echo file_get_contents($filename);
  } else {
    echo json_encode(["led" => "OFF"]);
  }
  exit;
}

http_response_code(405);
echo json_encode(["error" => "Use GET or PUT"]);
?>
