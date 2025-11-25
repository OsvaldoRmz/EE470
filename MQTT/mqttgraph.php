<?php
// ===== DB CONFIG =====
$host   = "mysql.hostinger.com";
$user   = "u926109375_db_OsvaldoRmz";
$pass   = "Osvaldo10010028";
$dbname = "u926109375_Osvaldoramirez";
$table  = "sensor_values";

$pdo = new PDO("mysql:host=$host;dbname=$dbname;charset=utf8", $user, $pass);

// Get last 50 samples (latest first), then reverse for left-to-right time
$query = $pdo->query("SELECT value, created_at FROM $table ORDER BY created_at DESC LIMIT 50");
$data  = $query->fetchAll(PDO::FETCH_ASSOC);

$data = array_reverse($data);
$labels = array_map(fn($row) => $row['created_at'], $data);
$values = array_map(fn($row) => (float)$row['value'], $data);
?>
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Potentiometer Data</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    body { font-family: Arial, sans-serif; margin: 20px; }
    .container { max-width: 900px; margin: 0 auto; }
    canvas { width: 100%; max-height: 400px; }
    table { border-collapse: collapse; width: 100%; margin-top: 20px; }
    th, td { border: 1px solid #ccc; padding: 6px 8px; text-align: center; }
    th { background: #f0f0f0; }
  </style>
</head>
<body>
<div class="container">
  <h1>Potentiometer Data from broker.hivemq.com</h1>
  <p>Data path: ESP → broker.hivemq.com → Python relay → MySQL (Hostinger) → this page.</p>

  <canvas id="potChart"></canvas>

  <h2>Last 50 Samples</h2>
  <table>
    <tr><th>Timestamp</th><th>Value</th></tr>
    <?php foreach ($data as $row): ?>
      <tr>
        <td><?= htmlspecialchars($row['created_at']) ?></td>
        <td><?= htmlspecialchars($row['value']) ?></td>
      </tr>
    <?php endforeach; ?>
  </table>
</div>

<script>
const labels = <?= json_encode($labels) ?>;
const values = <?= json_encode($values) ?>;

const ctx = document.getElementById('potChart').getContext('2d');
new Chart(ctx, {
  type: 'line',
  data: {
    labels: labels,
    datasets: [{
      label: 'Potentiometer Value',
      data: values,
      fill: false,
      tension: 0.1
    }]
  },
  options: {
    scales: {
      x: { title: { display: true, text: 'Time' } },
      y: { title: { display: true, text: 'Value (ADC reading)' } }
    }
  }
});
</script>
</body>
</html>
