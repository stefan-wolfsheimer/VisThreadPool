index = R"__HTML__(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <title>WebSocket Test</title>
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <style type="text/css">
    body {
      background-color: #789; margin: 0;
      padding: 0; font: 14px Helvetica, Arial, sans-serif;
    }
    div.content {
      width: 800px; margin: 2em auto; padding: 20px 50px;
      background-color: #fff; border-radius: 1em;
    }
    #messages {
      border: 2px solid #fec; border-radius: 1em;
      height: 10em; overflow: scroll; padding: 0.5em 1em;
    }
    a:link, a:visited { color: #69c; text-decoration: none; }
    @media (max-width: 700px) {
      body { background-color: #fff; }
      div.content {
        width: auto; margin: 0 auto; border-radius: 0;
        padding: 1em;
      }
    }
</style>

<script language="javascript" type="text/javascript">
  var getJSON = function(url, callback) {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', url, true);
    xhr.responseType = 'json';
    xhr.onload = function() {
      var status = xhr.status;
      if (status === 200) {
        callback(null, xhr.response);
      } else {
        callback(status, xhr.response);
      }
    };
    xhr.send();
  };
  var generateTable = function(err, data) {
     if(err === null) {
       var q = document.getElementById('queue');
       if(q) {
          q.innerHTML = '<p>' + data.queue.length + ' tasks queued</p>';
       }
       var innerHTML = '';
       var state2color = {
         'Waiting':         'grey',
         'Ready':           'grey',
         'Running':         'yellow',
         'CancelRequested': 'red',
         'Canceled':        'red',
         'Done':            'green',
         'Failed':          'red' 
       };
       var threadIndex = 0;
       data.threads.forEach(function(elem) {
         if(elem.state !== undefined) {
           innerHTML+= '<tr><td>Thread ' + threadIndex + '</td>';
           innerHTML+= '<td bgcolor="' + state2color[elem.state] + '"> TaskId: ' + elem.taskId + '</td>';
           innerHTML+= '<td bgcolor="' + state2color[elem.state] + '">' + elem.state + '</td></tr>';
         }
         else {
           innerHTML+= '<tr><td>Thread ' + threadIndex + '</td>';
           innerHTML+= '<td  bgcolor="grey">Ready</td></tr>';
         }
         threadIndex++;
       });
       document.getElementById('table').innerHTML = innerHTML;
     }
  };
  var ws = new WebSocket('ws://' + location.host + '/ws');

  if (!window.console) { window.console = { log: function() {} } };

  ws.onopen = function(ev)  { console.log(ev); };
  ws.onerror = function(ev) { console.log(ev); };
  ws.onclose = function(ev) { console.log(ev); };
  ws.onmessage = function(ev) {
    console.log(ev);
    var div = document.createElement('div');
    div.innerHTML = ev.data;
    var messages = document.getElementById('messages');
    messages.appendChild(div);
    messages.scrollTop = messages.scrollHeight;
    getJSON('http://' + location.host + '/list', generateTable);
  };

  window.onload = function() {
    document.getElementById('send_button').onclick = function(ev) {
      ws.send('');
    };
    getJSON('http://' + location.host + '/list', generateTable);
  };
</script>
</head>
<body>
  <div class="content">
    <h1>ThreadPool Demonstration</h1>
    Queue: <div id="queue"></div>
    <table id="table">
    </table>

    <div id="messages">
    </div>

    <p>
      <button id="send_button">Send Message</button>
    </p>
  </div>
</body>
</html>
)__HTML__";
