const url = location.hostname;
const conn = new WebSocket(`ws://${url}:81/`, ['arduino']);

const utils = {

};

const Slider = function({color, colorValue, handleSliderColorChange}) {
  return (
    <div className="slide">{color}:
      <input id={color[0]} name={color} className="slider" type="range" min="0" max="255" step="1" onChange={handleSliderColorChange} value={colorValue} />
    </div>
  );
};

const SetColors = function({handleColorChange}) {
  return (
    <div className="setColors">
      {Object.keys(setColors).map(function(color, i) {
        const colorHex = setColors[color];
        const hexStyle = {
          background: colorHex,
          color: (utils.getLuma(colorHex) < 100) ? '#fff' : '#000'
        };
        return (<button key={i} style={hexStyle} onClick={handleColorChange.bind(null, colorHex)}>{color}</button>);
      })}
    </div>
  );
};

const App = React.createClass({
  handleError(err) {
    console.log('Error ', error);
  },
  componentWillMount() {
    conn.onopen = () => {
      conn.send('Connect ' + new Date());
    };

    conn.onerror = (error) => {
      this.handleError(error);
    };

    conn.onmessage = (e) => {
      this.handleUpdate(e.data)
    };
  },
  handleUpdate(data) {
    console.log(data);
  },
  handleSend(data) {
    conn.send(data);
  },
  render() {
    return (
      <div className="app">
        <h1>KegBot</h1>
      </div>
    );
  }
});

ReactDOM.render(
  <App/>,
  document.querySelector('.container')
);
