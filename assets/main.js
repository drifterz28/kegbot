//const url = '192.168.0.108'; // for local testing
const url = location.hostname;
const kegBotUrl = 'https://iot-ecommsolution.rhcloud.com/kegbot';
const conn = new WebSocket(`ws://${url}:81/`, ['arduino']);

// notes
// 1 gallon = 8lbs = 128oz by weight
// empty keg ~165 Oz
// full keg ~928
// 5 gallons ~640
const utils = {
  precentOfMax: (amount, maxAmount) => {
    if(amount === 0 && maxAmount === 0) {
      return 0;
    }
    return (amount / maxAmount) * 100;
  },
  updateKeg: (kegInfo, callback) => {
    const {gallons, kegNumber, maxValue, name} = kegInfo;
    fetch(`${kegBotUrl}?keg=${kegNumber}&name=${name}&gallons=${gallons}&maxValue=${maxValue}`)
      .then(response => response.json())
      .then(json => {
        callback(json);
      });
  },
  getKegInfo: (callback) => {
    fetch(`${kegBotUrl}`)
      .then(response => {
        return response.json().then(json => {
          callback(json);
        });
      });
  },
  launchIntoFullscreen: () => {
    const element = document.documentElement;
    if(element.requestFullscreen) {
      element.requestFullscreen();
    } else if(element.webkitRequestFullscreen) {
      element.webkitRequestFullscreen();
    }
  }

};

const Keg = ({value, maxValue, name, kegNumber, handleEdit}) => {
  const max = (maxValue < value)? value : maxValue;
  const precentage = Math.floor(utils.precentOfMax(value, max));
  const styles = {
    "transform": 'translateY(-' + precentage + '%)'
  };
  const edit = () => {
    handleEdit(kegNumber);
  };
  return (
    <section className="kegWrapper">
      <h1 onClick={edit} className="brewName">{name}</h1>
      <div className="kegGraph">
        <div className="keg">
          {precentage}%
        </div>
        <div className="progress" style={styles}></div>
      </div>
    </section>
  );
};

const TempAndHumidity = ({temp, humidity, status, handleFan, fan, handleFullScreen}) => {
  const connection = status ? 'connected' : 'notConnected';
  const fanStatus = fan ? 'fanOn' : '';
  return (
    <section className={`tempAndHumidity ${connection}`}>
      <div onClick={handleFan} className={`temp ${fanStatus}`}>
        <small>Temp</small>
        {Math.round(temp)}
      </div>
      <div onClick={handleFullScreen} className="humidity">
        <small>Humidity</small>
        {Math.round(humidity)}
      </div>
    </section>
  );
};

const Modal = React.createClass({
  getInitialState() {
    const kegNumber = this.props.kegEdit === 1 ? 'kegOne' : 'kegTwo';
    return {
      kegNumber: kegNumber,
      name: this.props[`${kegNumber}Name`],
      maxValue: this.props[`${kegNumber}MaxValue`],
      gallons: this.props[`${kegNumber}Gallons`]
    }
  },
  handleSetMax() {
    this.setState({
      maxValue: this.props[`${this.state.kegNumber}`]
    });
  },
  handleChange(e) {
    const target = e.target;
    let newState = {};
    newState[target.name] = target.value;
    this.setState(newState);
  },
  handleSave(e) {
    e.preventDefault();
    this.props.handleSave(this.state);
  },
  render() {
    return (
      <div className="modal">
        <div className="modalContent">
          <button onClick={this.props.modalClose} className="close">x</button>
          <form onSubmit={this.handleSave} className="kegForm">
            <h1>Edit {this.state.kegNumber}</h1>
            <label>
              Name: <input value={this.state.name} name="name" onChange={this.handleChange}/>
            </label>
            <label>
              Max Value:
              <button type="button" className="setMaxBtn" onClick={this.handleSetMax}>Set Max</button>
              <input className="setMax" value={this.state.maxValue} name="maxValue" onChange={this.handleChange}/>
            </label>
            <label>
              Gallons: <input type="number" value={this.state.gallons} name="gallons" onChange={this.handleChange}/>
            </label>
            <button>Save</button>
          </form>
        </div>
      </div>
    );
  }
});

const App = React.createClass({
  getInitialState() {
    return {
      isConnected: false,
      modalOpen: false,
      kegEdit: null,
      fan: 0,
      temp: 0,
      humidity: 0,
      kegOne: 0,
      kegTwo: 0,
      kegOneName: '',
      kegTwoName: '',
      kegOneMaxValue: 0,
      kegTwoMaxValue: 0,
      kegOneGallons: 0,
      kegTwoGallons: 0
    };
  },
  handleError(error) {
    console.log('Error ', error);
    this.setState({isConnected: false});
  },
  componentWillMount() {
    conn.onopen = () => {
      conn.send('Connect ' + new Date());
    };
    conn.onclose = (close) => {
      console.log(close);
    };
    conn.onerror = (error) => {
      this.handleError(error);
    };
    conn.onmessage = (e) => {
      this.handleUpdate(e.data);
    };
    utils.getKegInfo(this.updateKegs);
  },
  handleUpdate(data) {
    if(data === 'Connected') {
      this.setState({isConnected: true});
    } else {
      const json = JSON.parse(data);
      this.setState(json);
    }
  },
  handleFan() {
    let fanState = 0;
    if(this.state.fan == 0) {
      fanState = 1;
    }
    conn.send(fanState);
  },
  handleFullScreen() {
    console.log('fullscreen');
    utils.launchIntoFullscreen();
  },
  kegMaxValue(max, gallons) {
    const onces = gallons * 128;
    const weightOfKeg = onces - max;
    return max - weightOfKeg;
  },
  updateKegs(json) {
    const {kegOne, kegTwo} = json;
    this.setState({
      kegOneName: kegOne.name,
      kegTwoName: kegTwo.name,
      kegOneMaxValue: kegOne.maxValue,
      kegTwoMaxValue: kegTwo.maxValue,
      kegOneGallons: kegOne.gallons,
      kegTwoGallons: kegTwo.gallons,
      modalOpen: false
    });
  },
  handleSave(kegInfo) {
    utils.updateKeg(kegInfo, this.updateKegs);
  },
  handleEdit(kegNumber) {
    this.setState({
      modalOpen: true,
      kegEdit: kegNumber
    });
  },
  handleModalClose() {
    this.setState({
      modalOpen: false
    });
  },
  render() {
    const {
      temp,
      humidity,
      kegOne,
      kegTwo,
      kegOneName,
      kegTwoName,
      kegOneMaxValue,
      kegTwoMaxValue,
      isConnected,
      modalOpen,
      kegOneGallons,
      kegTwoGallons
    } = this.state;
    const newKegOneMax = this.kegMaxValue(kegOneMaxValue, kegOneGallons);
    const newKegTwoMax = this.kegMaxValue(kegTwoMaxValue, kegTwoGallons);
    return (
      <div className="app">
        <Keg name={kegOneName} value={kegOne} kegNumber={1} maxValue={newKegOneMax} handleEdit={this.handleEdit}/>
        <TempAndHumidity
          temp={temp}
          humidity={humidity}
          status={isConnected}
          fan={this.state.fan}
          handleFullScreen={this.handleFullScreen}
          handleFan={this.handleFan}
        />
        <Keg name={kegTwoName} value={kegTwo} kegNumber={2} maxValue={newKegTwoMax} handleEdit={this.handleEdit}/>
        {modalOpen && <Modal {...this.state} handleSave={this.handleSave} modalClose={this.handleModalClose}/>}
      </div>
    );
  }
});

ReactDOM.render(
  <App/>,
  document.querySelector('.container')
);
