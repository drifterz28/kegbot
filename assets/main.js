const url = '192.168.0.108'; //location.hostname;
const conn = new WebSocket(`ws://${url}:81/`, ['arduino']);

const utils = {
  precentOfMax: (amount, maxAmount) => {
    return (amount / maxAmount) * 100;
  }
};

const Keg = React.createClass({
  getInitialState() {
    return {
      isEdit: false,
      kegName: this.props.name,
    };
  },
  handleNameChange(e) {
    this.setState({
      isEdit: true
    });
  },
  handleChange(e) {
    this.setState({
      kegName: e.target.value
    });
  },
  handleSave(e) {
    e.preventDefault();
    this.setState({
      isEdit: false
    });
    this.props.handleSave(this.state.kegName, this.props.kegNumber);
  },
  render() {
    const {value} = this.props;
    const {kegName} = this.state;
    const styles = {
      "transform": 'translateY(-' + value + '%)'
    };
    return (
      <section className="kegWrapper">
        {!this.state.isEdit ?
          <h1 onDoubleClick={this.handleNameChange} className="brewName">{kegName}</h1> :
          <form className="nameChange" onSubmit={this.handleSave}>
            <input onChange={this.handleChange} defaultValue={kegName}/>
          </form>
        }
        <div className="kegGraph">
          <div className="keg">
            {Math.ceil(value)}%
          </div>
          <div className="progress" style={styles}></div>
        </div>
      </section>
    );
  }
});

const TempAndHumidity = function({temp, humidity}) {
  return (
    <section className="tempAndHumidity">
      <div className="temp">
        <small>Temp</small>
        {Math.round(temp)}
      </div>
      <div className="humidity">
        <small>Humidity</small>
        {Math.round(humidity)}
      </div>
    </section>
  );
};

const App = React.createClass({
  getInitialState() {
    return {
      "temp": 0,
      "humidity": 0,
      "kegOne": 0,
      "kegTwo": 0,
      "kegOneName": 'dbl click to add name',
      "kegTwoName": 'dbl click to add name'
    };
  },
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
      this.handleUpdate(e.data);
    };
  },
  handleUpdate(data) {
    const json = JSON.parse(data);
    this.setState(json);
  },
  handleSave(value, number) {
    let newState = {};
    if(number === 1) {
      newState.kegOneName = value;
    } else {
      newState.kegTwoName = value;
    }
    this.setState(newState);
  },
  render() {
    const {humidity, kegOne, kegOneName, kegTwo, kegTwoName, temp} = this.state;
    return (
      <div className="app">
        <Keg name={kegOneName} value={kegOne} kegNumber='1' handleSave={this.handleSave}/>
        <TempAndHumidity temp={temp} humidity={humidity}/>
        <Keg name={kegTwoName} value={kegTwo} kegNumber='2' handleSave={this.handleSave}/>
      </div>
    );
  }
});

ReactDOM.render(
  <App/>,
  document.querySelector('.container')
);
