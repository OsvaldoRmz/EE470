<?php
$FILE = __DIR__ . "/results.txt";

// Load existing JSON state or create a default one
if (file_exists($FILE)) {
  $data = json_decode(file_get_contents($FILE), true);
  if (!is_array($data)) $data = ["led" => "OFF", "rgb" => 0];
} else {
  $data = ["led" => "OFF", "rgb" => 0];
}

// Handle RGB slider save
if ($_SERVER["REQUEST_METHOD"] === "POST" && isset($_POST["rgb"])) {
  $val = intval($_POST["rgb"]);
  $data["rgb"] = max(0, min(255, $val)); // clamp 0–255
  file_put_contents($FILE, json_encode($data, JSON_PRETTY_PRINT));
  header("Location: " . $_SERVER["PHP_SELF"]); // reload page
  exit;
}
?>
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <title>RGB LED Control</title>
  <style>
    body { font-family: system-ui, sans-serif; margin: 24px; }
    input[type=range]{ width: 320px; }
    .preview { width: 40px; height: 20px; border: 1px solid #aaa; display: inline-block; vertical-align: middle; border-radius: 4px; margin-left: 8px; }
  </style>
</head>
<body>
  <h1>RGB LED Control</h1>
  <p>Current RGB Value: <strong id="val"><?php echo intval($data["rgb"]); ?></strong></p>
  <span class="preview" id="pv" style="background: rgb(<?php echo intval($data["rgb"]); ?>,0,0);"></span>

  <form method="post">
    <input type="range" name="rgb" min="0" max="255" value="<?php echo intval($data["rgb"]); ?>"
      oninput="document.getElementById('val').textContent=this.value;
               document.getElementById('pv').style.background='rgb('+this.value+',0,0)'">
    <button type="submit">Save</button>
  </form>

</body>
</html>
