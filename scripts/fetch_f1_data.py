"""
Fetches F1 season data from the Jolpica-F1 API (the Ergast replacement)
and writes it into the flat JSON files the C++ DataImporter expects.

Stdlib only - no pip install required.

Usage:
    python scripts/fetch_f1_data.py                     # 2024 season -> data/
    python scripts/fetch_f1_data.py --season 2012        # 2012 season -> data_2012/
    python scripts/fetch_f1_data.py --season 2012 --out data_2012
"""
import argparse
import json
import os
import urllib.request
import urllib.error

BASE_URL = "https://api.jolpi.ca/ergast/f1"
REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

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


def fetch_drivers(season):
    data = fetch_json(f"{BASE_URL}/{season}/drivers.json")
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


def fetch_teams(season):
    data = fetch_json(f"{BASE_URL}/{season}/constructors.json")
    constructors = data["MRData"]["ConstructorTable"]["Constructors"]
    result = []
    for c in constructors:
        result.append({
            "name": c.get("name", ""),
            "pit_stop_variance": 0.0,
        })
    return result


def fetch_circuits(season):
    data = fetch_json(f"{BASE_URL}/{season}/circuits.json")
    circuits = data["MRData"]["CircuitTable"]["Circuits"]
    result = []
    for c in circuits:
        result.append({
            "name": c.get("circuitName", ""),
            "base_degradation_rate": 0.0,
        })
    return result


def fetch_results(season):
    # The API silently caps `limit` at 100 per request, so we must page
    # through with `offset` until we've collected everything.
    result = []
    offset = 0
    limit = 100
    while True:
        data = fetch_json(f"{BASE_URL}/{season}/results.json?limit={limit}&offset={offset}")
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


def write_json(data_dir, filename, payload):
    os.makedirs(data_dir, exist_ok=True)
    path = os.path.join(data_dir, filename)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(payload, f, indent=2)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--season", default="2024", help="F1 season year to fetch (default: 2024)")
    parser.add_argument("--out", default=None, help="Output directory, relative to repo root (default: 'data' for 2024, 'data_<season>' otherwise)")
    args = parser.parse_args()

    season = args.season
    out_name = args.out or ("data" if season == "2024" else f"data_{season}")
    data_dir = os.path.join(REPO_ROOT, out_name)

    drivers = fetch_drivers(season)
    write_json(data_dir, "drivers.json", drivers)

    teams = fetch_teams(season)
    write_json(data_dir, "teams.json", teams)

    circuits = fetch_circuits(season)
    write_json(data_dir, "circuits.json", circuits)

    results = fetch_results(season)
    write_json(data_dir, "results.json", results)

    print(f"Season {season} -> {data_dir}")
    print(f"Fetched {len(drivers)} drivers, {len(teams)} teams, "
          f"{len(circuits)} circuits, {len(results)} race results.")


if __name__ == "__main__":
    main()
