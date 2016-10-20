const url = '192.168.0.108'; //location.hostname;
const kegBotUrl = 'https://iot-ecommsolution.rhcloud.com/kegbot';
const conn = new WebSocket(`ws://${url}:81/`, ['arduino']);

const utils = {
  precentOfMax: (amount, maxAmount) => {
    if(amount === 0 && maxAmount === 0) {
      return 0;
    }
    return (amount / maxAmount) * 100;
  },
  updateName: (kegNumber, kegName) => {
    return fetch(`${kegBotUrl}?keg=${kegNumber}&name=${kegName}`)
      .then(response => response.json())
      .then(json => json);
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
  handleNewMaxValue() {
    this.props.handleNewMaxValue(this.props.kegNumber);
  },
  render() {
    const {value, maxValue} = this.props;
    const {kegName} = this.state;
    const max = (maxValue < value)? value : maxValue;
    const precentage = Math.floor(utils.precentOfMax(value, max));
    const styles = {
      "transform": 'translateY(-' + precentage + '%)'
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
          <div className="keg" onDoubleClick={this.handleNewMaxValue}>
            {precentage}%
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
      temp: 0,
      humidity: 0,
      kegOne: 0,
      kegTwo: 0,
      kegOneName: 'dbl click to add name',
      kegTwoName: 'dbl click to add name',
      kegOneMaxValue: 0,
      kegTwoMaxValue: 0
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
    console.log(json)
    this.setState(json);
  },
  handleSave(value, number) {
    const kegNumber = (number === 1) ? 'kegOne' : 'kegTwo';
    const newState = utils.updateName(kegNumber, value);
    console.log(newState)
    this.setState(newState);
  },
  handleNewMaxValue(kegNumber) {
    console.log(kegNumber);
  },
  render() {
    const {temp, humidity, kegOne, kegTwo, kegOneName, kegTwoName, kegOneMaxValue, kegTwoMaxValue} = this.state;
    return (
      <div className="app">
        <Keg name={kegOneName} value={kegOne} kegNumber={1} maxValue={kegOneMaxValue} handleSave={this.handleSave} handleNewMaxValue={this.handleNewMaxValue}/>
        <TempAndHumidity temp={temp} humidity={humidity}/>
        <Keg name={kegTwoName} value={kegTwo} kegNumber={2} maxValue={kegTwoMaxValue} handleSave={this.handleSave} handleNewMaxValue={this.handleNewMaxValue}/>
      </div>
    );
  }
});

ReactDOM.render(
  <App/>,
  document.querySelector('.container')
);
