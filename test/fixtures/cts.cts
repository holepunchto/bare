interface Greeting {
  message: string
}

const greeting: Greeting = { message: 'Hello from CTS' }

module.exports = greeting.message
