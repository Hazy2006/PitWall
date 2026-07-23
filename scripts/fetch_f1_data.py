"""
Fetches 2024 F1 season data from the Jolpica-F1 API (the Ergast replacement)
and writes it into the flat JSON files the C++ DataImporter expects.

Stdlib only - no pip install required.
"""
import json
import os
import urllib.request
import urllib.error

BASE_URL = "https://api.jolpi.ca/ergast/f1"
DATA_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "data")

REQUEST_HEADERS = {
    "User-Agent": "PitWall-DataFetcher/1.0",
    "Accept": "application/json",
}


def fetch_json(url):
    request = urllib.request.Request(url, headers=REQUEST_HEADERS)
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            return json.loads(response.read().decode("utf-8"))
    except urllib.error.URLError as exc:
        raise RuntimeError(f"Failed to fetch {url}: {exc}") from exc


def to_number(value, default=0):
    try:
        return int(value)
    except (TypeError, ValueError):
        return default


def fetch_drivers():
    data = fetch_json(f"{BASE_URL}/2024/drivers.json")
    drivers = data["MRData"]["DriverTable"]["Drivers"]
    result = []
    for d in drivers:
        name = f"{d.get('givenName', '')} {d.get('familyName', '')}".strip()
        result.append({
            "name": name,
            "tire_management_modifier": 0.0,
            "base_pace_delta": 0.0,
        })
    return result


def fetch_teams():
    data = fetch_json(f"{BASE_URL}/2024/constructors.json")
    constructors = data["MRData"]["ConstructorTable"]["Constructors"]
    result = []
    for c in constructors:
        result.append({
            "name": c.get("name", ""),
            "pit_stop_variance": 0.0,
        })
    return result


def fetch_circuits():
    data = fetch_json(f"{BASE_URL}/2024/circuits.json")
    circuits = data["MRData"]["CircuitTable"]["Circuits"]
    result = []
    for c in circuits:
        result.append({
            "name": c.get("circuitName", ""),
            "base_degradation_rate": 0.0,
        })
    return result


def fetch_results():
    # The API silently caps `limit` at 100 per request, so we must page
    # through with `offset` until we've collected everything.
    result = []
    offset = 0
    limit = 100
    while True:
        data = fetch_json(f"{BASE_URL}/2024/results.json?limit={limit}&offset={offset}")
        mrdata = data["MRData"]
        for race in mrdata["RaceTable"]["Races"]:
            circuit_name = race.get("Circuit", {}).get("circuitName", "")
            for r in race.get("Results", []):
                driver = r.get("Driver", {})
                constructor = r.get("Constructor", {})
                driver_name = f"{driver.get('givenName', '')} {driver.get('familyName', '')}".strip()
                result.append({
                    "driver_name": driver_name,
                    "circuit_name": circuit_name,
                    "position": to_number(r.get("position")),
                    "grid": to_number(r.get("grid")),
                    "team_name": constructor.get("name", ""),
                })
        total = to_number(mrdata.get("total"))
        offset += limit
        if offset >= total:
            break
    return result


def write_json(filename, payload):
    os.makedirs(DATA_DIR, exist_ok=True)
    path = os.path.join(DATA_DIR, filename)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(payload, f, indent=2)


def main():
    drivers = fetch_drivers()
    write_json("drivers.json", drivers)

    teams = fetch_teams()
    write_json("teams.json", teams)

    circuits = fetch_circuits()
    write_json("circuits.json", circuits)

    results = fetch_results()
    write_json("results.json", results)

    print(f"Fetched {len(drivers)} drivers, {len(teams)} teams, "
          f"{len(circuits)} circuits, {len(results)} race results.")


if __name__ == "__main__":
    main()
